// Copyright (C) 2017 BRK Brands, Inc. All Rights Reserved.
/// @file
///
/// The HTTP API does the socket I/O to send request to and receive response
/// from Exosite. If a response has HTTP status 200, the API also prepares the
/// buffer for the content in the response.

#include "fa_log.h"
#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <stddef.h>

/// Return a string constant coresponding to the \p action.
/// @param[in] action the HTTP action
/// @return a constant string to used in an HTTP request for the given action.
static const char *httpActionString(HttpAction action)
{
    switch (action) {
    case HTTP_HEAD:   return "HEAD";
    case HTTP_GET:    return "GET";
    case HTTP_POST:   return "POST";
    case HTTP_PUT:    return "PUT";
    case HTTP_DELETE: return "DELETE";
    }
    assert(false);
}

/// Add a single character to a buffer. At entry, \p start should be less than
/// or equal to \p end.
/// @param[in] start Pointer to the next available character in the buffer.
/// @param[in] end Pointer to the character just after the end of the buffer.
/// @param[in] c Character to add to the buffer.
/// @return Pointer to the character after the last character that was added to
///         the buffer.
static char *addChar(char *start, char *end, char c)
{
    if (start != NULL && start < end) {
        *start = c;
        ++start;
    }
    return start;
}

/// Add a string to a buffer. At entry, \p start should be less than or equal to
/// \p end.
/// @param[in] start Pointer to the next available character in the buffer.
/// @param[in] end Pointer to the character just after the end of the buffer.
/// @param[in] source The nul-terminated string to add to the buffer
/// @return Pointer to the character after the last character that was added to
///         the buffer.
static char *addString(char *start, char *end, const char *source)
{
    if (start != NULL && source != NULL) {
        while (start < end) {
            *start = *source;
            if (*source == '\0') {
                break;
            }
            ++start;
            ++source;
        }
    }
    return start;
}

/// Add an HTTP header to the request buffer. At entry, \p dest must be less
/// than or equal to \p end. According to RFC 2616, section 4.2, the header
/// format is: field-name ":" [ field-value ]
/// @param[in] dest Pointer to the next available character in the buffer.
/// @param[in] end Pointer to the character just after the end of the buffer.
/// @param[in] name The Name of the header (e.g. "Content-Length")
/// @param[in] value The value associated with the header.
/// @return Pointer to the character after the last character that was added to
///         the buffer.
static char *addHeader(char *dest, char *end, const char *name, const char *value)
{
    dest = addString(dest, end, name);
    dest = addString(dest, end, ": ");
    dest = addString(dest, end, value);
    dest = addString(dest, end, "\r\n");
    return dest;
}

/// Build up an HTTP request in the buffer. Automatically add certain mandatory
/// headers so the caller does not need to remember them.
/// @param[in] action the kind of request to make: GET,POST,etc.
/// @param[in] uri the URI being requested.
/// @param[in] params optional pointer to a list of name/value pairs to encode
///          as part of the request. Use NULL if no extra parameters are needed.
///          The list should end with a pair that contains 2 NULLs. Pass NULL if
///          no query parameters are needed.
/// @param[in] extraHeaders optional pointer to a list of name/value pairs to
///          use as extra headers in the request. For POST requests, the
///          Content-Length header is automatically included. Use NULL if no
///          extra headers are needed. The list should end with a pair that
///          contains 2 NULLs.
static bool makeHttpRequest(HttpConnection *connection, HttpAction action,
                            const char *uri, HttpPair *params,
                            HttpPair *extraHeaders,
                            const char *body, char *buffer, uint32_t bufferSize)
{
    char *dest = buffer;
    char *end = &buffer[bufferSize];

    const char *host = connection->host;
    int port = connection->port;
    assert(uri);
    assert(host);
    assert(port != 0);

    // Begin the request with the action and the URI.
    dest = addString(dest, end, httpActionString(action));
    dest = addChar(dest, end, ' ');
    dest = addString(dest, end, uri);

    // Are there any query parameters to add to the URI?
    if (params) {
        char prefix = '?';
        HttpPair *p = params;
        while (p->name != NULL) {
            dest = addChar(dest, end, prefix);
            dest = addString(dest, end, p->name);
            if (p->value != NULL) {
                dest = addChar(dest, end, '=');
                dest = addString(dest, end, p->value);
            }
            prefix = '&';
            p++;
        }
    }

    // Add the HTTP version.
    dest = addString(dest, end, " HTTP/1.1\r\n");

    // Header: "Host: <hostname>[:<port>]"
    dest = addString(dest, end, "Host: ");
    dest = addString(dest, end, host);
    // See if we need to also include the port number
    if (port != 80) {
        char value_buffer[11];
        snprintf(value_buffer, sizeof(value_buffer), "%u", port);
        dest = addChar(dest, end, ':');
        dest = addString(dest, end, value_buffer);
    }
    dest = addString(dest, end, "\r\n");

    // For POST requests, add the Content-Length header
    uint32_t body_size = 0;
    if (body != NULL) {
        body_size = (uint32_t)strlen(body);
    }
    if (action == HTTP_POST && body != NULL) {
        char value_buffer[11];
        snprintf(value_buffer, sizeof(value_buffer), "%u", body_size);
        dest = addHeader(dest, end, "Content-Length", value_buffer);
    }

    // Add any extra headers
    if (extraHeaders) {
        HttpPair *p = extraHeaders;
        while (p->name && p->value) {
            dest = addHeader(dest, end, p->name, p->value);
            p++;
        }
    }

    // Add the final blank line
    dest = addString(dest, end, "\r\n");

    // make sure we did not overflow the buffer.
    if (dest >= end) {
        FA_ERROR("The request was too long for our buffer.");
        return false;
    }
    // Make sure the request is NULL terminated.
    *dest = '\0';
    FA_INFO("Request:\n%s", buffer);
    return true;
}

/// Send a full HTTP request on the connection. An attempt is made to re-use an
/// open connection. If the connection is not open, it is opened and the request
/// is sent. If the connection was open and there was a problem writing to the
/// connection, the connection is closed and we try again.
/// @param[in,out] connection the connection information.
/// @param[in] request the HTTP request, including a final blank line.
/// @param[in] body optional body to send with the request.
/// @return true If the conenction was successfully opened (if needed) and the
///              request and body sent on the connection.
static bool sendHttpRequest(HttpConnection *connection, const char *request, const char *body)
{
    char error_buffer[128];
    error_buffer[0] = '\0';

    bool reuseConnection = true;
    struct mg_connection *conn = connection->connection;
    if (conn == NULL) {
        reuseConnection = false;
        const char *host = connection->host;
        int port = connection->port;

        // Create a new connection
        conn = mg_connect_client(host, port, 0, error_buffer, sizeof(error_buffer));
        connection->connection = conn;

        if (conn == NULL) {
            FA_ERROR("%s: Problem creating connection to %s:%d - %s", __func__, host, port, error_buffer);
            return false;
        }
    }

    int status;
    uint32_t size = (uint32_t)strlen(request);
    status = mg_write(conn, request, size);
    if (status < 0) {
        FA_ERROR("%s: Error sending the headers.", __func__);
        if (reuseConnection) {
            // We tried to re-use the connection, but had a problem.
            // Close this connection and try again.
            FA_ERROR("Closing the connection and trying again...");
            closeHttpConnection(connection);
            return sendHttpRequest(connection, request, body);
        }
        return false;
    }

    if (body != NULL) {
        // We have a request body to send.
        size = (uint32_t)strlen(body);
        status = mg_write(conn, body, size);
        if (status < 0) {
            FA_ERROR("%s: Error sending the body of the request.", __func__);
            return false;
        }
    }
    return true;
}

/// Read from the connection until we have received the response headers. Parse
/// the status code and headers so they are available for easy access.
/// @param[in,out] connection pointer to the connection information.
/// @return true if an HTTP response (not including the body) was received.
static bool getHttpResponseHeaders(HttpConnection *connection)
{
    char error_buffer[128];
    int status;
    struct mg_connection *conn = connection->connection;
    status = mg_get_response(conn, error_buffer, sizeof(error_buffer), connection->timeout);
    if (status < 0) {
        FA_ERROR("%s: Problem getting response from %s:%d - %s", __func__,
                 connection->host, connection->port, error_buffer);
        return false;
    }
    // Get the status code. CivetWeb does not store the code in a reasonable
    // place, so we will convert it to our HttpStatus type and store it in
    // the connection.
    const struct mg_request_info *info = mg_get_request_info(conn);
    status = atoi(info->uri);
    connection->status_code = (HttpStatus)status;
    FA_INFO("Request status: %s   %d  ***********************", info->uri, status);
    return true;
}

// Send an HTTP request with an optional body and fill in the response
// information.
// @param[in,out] connection pointer to the connection information.
// @param[in] action the kind of request to make: GET,POST,etc.
// @param[in] uri the URI being requested.
// @param[in] params optional pointer to a list of name/value pairs to encode
//          as part of the request. Use NULL if no extra parameters are needed.
//          The list should end with a pair that contains 2 NULLs.
// @param[in] extraHeaders optional pointer to a list of name/value pairs to
//          use as extra headers in the request. For POST requests, the
//          content-length header is automatically included. Use NULL if no
//          extra headers are needed. The list should end with a pair that
//          contains 2 NULLs.
// @param[in/out] response pointer to the structure where the response data and
//          size is to be stored.
// @return the \ref HttpStatus corresponding to the code returned by the server.
HttpStatus beginHttpRequest(HttpConnection *connection, HttpAction action,
                            const char *uri, HttpPair *params,
                            HttpPair *extraHeaders, const char *body)
{
    if (connection == NULL) {
        return HTTP_INVALID;
    }
    connection->status_code = HTTP_INVALID;

    // Write the HTTP request to the buffer.
    char buffer[1024];
    if (!makeHttpRequest(connection, action, uri, params, extraHeaders, body, buffer, sizeof(buffer))) {
        return HTTP_INVALID;
    }

    /// Start the connection with the header and body.
    if (!sendHttpRequest(connection, buffer, body)) {
        return HTTP_INVALID;
    }

    // Wait for the response code from the server.
    if (!getHttpResponseHeaders(connection)) {
        return HTTP_INVALID;
    }
    return connection->status_code;
}

// Get information about the response to the HTTP request. If the response code
// is a success code, the body of the response will be returned as part of the
// ResponseData. If there was a problem, the body of the response will be ignored.
// Return the http status code from the response.
HttpStatus extractResponseBody(HttpConnection *connection, ResponseData *response)
{
    if (connection == NULL) {
        return HTTP_INVALID;
    }
    struct mg_connection *conn = connection->connection;
    const struct mg_request_info *info = mg_get_request_info(conn);
    response->size = (uint32_t)info->content_length;
    char *data = calloc(1,response->size+1);
    response->data = data;
    size_t to_read = response->size;
    while (to_read > 0) {
        ssize_t num_read = mg_read(conn, data, to_read);
        if (num_read <= 0) {
            return HTTP_INVALID;
        }
        to_read -= (size_t)num_read;
        data += num_read;
    }
    return (HttpStatus)connection->status_code;
}

// Get binary blob data for the HTTP request from server up to 1MB size.
// The actual size of data will be saved in response for each call.
// Return the http status code from the response.
HttpStatus extractBinaryResponseBody(HttpConnection *connection, ResponseData *response)
{
    if (connection == NULL) {
        return HTTP_INVALID;
    }
    struct mg_connection *conn = connection->connection;
    memset(response->data, 0, DL_BLOB_CHUNK_SIZE);
    response->size = 0;

    size_t to_read = DL_BLOB_CHUNK_SIZE;
    while (to_read > 0) {
        ssize_t num_read = mg_read(conn, response->data, to_read);
        if (num_read <= 0) {
            return HTTP_INVALID;
        }
        to_read -= (size_t)num_read;
        response->data += num_read;
        response->size += num_read;
    }
    return (HttpStatus)connection->status_code;
}

// Return a pointer to the string value associated with a response header. NULL
// will be returned if a problem was encountered or the header was not returned
// as part of the response. The caller should use or copy the value before
// calling another function in this module.
const char *getResponseHeader(HttpConnection *connection, const char *headerName)
{
    return mg_get_header(connection->connection, headerName);
}

/// Close the socket associated with the connection. This must be done
/// explicitly so we can later support doing multiple requests via the same
/// connection.
/// @param[in] connection pointer to the HttpConnection data returned by \ref
///                       beginHttpRequest.
void closeHttpConnection(HttpConnection *connection)
{
    if (connection != NULL) {
        mg_close_connection(connection->connection);
        connection->connection = NULL;
    }
}

static HttpStatus httpRequest (const char *hostname, const int port,
                        const char *uri, HttpAction action,
                        HttpPair *queryList, HttpPair *extraHeaders,
                        ResponseData *rawData, const char *body)
{
    HttpStatus result;

    HttpConnection connection = HTTP_CONNECTION(hostname, port);
    result = beginHttpRequest(&connection, action, uri,
                              queryList, extraHeaders, body);
    if (result != HTTP_INVALID) {
        result = extractResponseBody(&connection, rawData);
        if (result == HTTP_OK) {
            FA_NOTICE("Read from body:%s", rawData->data);
        } else if (result == HTTP_NO_CONTENT) {
            FA_NOTICE("Request success code: %d", result);
        } else  {
            FA_ERROR("Failed to read data! code: %d", result);
        }
    }
    closeHttpConnection(&connection);
    return result;
}

HttpStatus httpGetRequest (const char *hostname, const int port, const char *uri,
                        HttpPair *queryList, HttpPair *extraHeaders,
                        ResponseData *rawData, const char *body)
{

    return httpRequest(hostname, port, uri, HTTP_GET, queryList, extraHeaders, rawData, body);
}

HttpStatus httpPostRequest (const char *hostname, const int port, const char *uri,
                        HttpPair *extraHeaders, ResponseData *rawData, const char *body)
{

    return httpRequest(hostname, port, uri, HTTP_POST, NULL, extraHeaders, rawData, body);
}
