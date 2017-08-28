// Copyright (C) 2017 BRK Brands, Inc. All Rights Reserved.

/// @file
/// The HTTP API handles GET and POST request and parsing the response
/// from Exosite HTTP API. It would also be good to make this generic so
/// accessing other servers can be done with the same code.


#ifndef SRC_HTTP_H
#define SRC_HTTP_H

#include <stdint.h>
#include <civetweb.h>

// DEFINES ///////////////////////////////////////////////////////////////////


#define CONTENT_JSON            "application/json"

/// Default timeout in milliseconds for our HTTP requests.
#define HTTP_DEFAULT_TIMEOUT (60*1000) //  (60 seconds)

/// Receive binary blob in 1MB chunk
#define DL_BLOB_CHUNK_SIZE      (1024*1024)

/// The various HTTP actions.
typedef enum {
    HTTP_HEAD,
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
} HttpAction;

/// Most possible status code returned from Exosite or our HLS server.
typedef enum {
    HTTP_INVALID         = 0,
    HTTP_OK              = 200,
    HTTP_NO_CONTENT      = 204,
    HTTP_RESET_CONTENT   = 205,
    HTTP_PARTIAL_CONTENT = 206,
    HTTP_NOT_MODIFIED    = 304,
    HTTP_BAD_REQUEST     = 400,
    HTTP_UNAUTHORIZED    = 401,
    HTTP_FORBIDDEN       = 403,
    HTTP_NOT_FOUND       = 404,
    HTTP_CONFLICT        = 409,
    HTTP_SERVER_ERROR    = 500
} HttpStatus;

/// Information about the body of an HTTP response. If data is != NULL, the
/// caller is responsible for freeing the memory pointed to by data.
typedef struct ResponseData {
    char *data;
    uint32_t size;
} ResponseData;

/// Used to hold extra information like HTTP headers and their values.
typedef struct {
    const char *name;
    const char *value;
} HttpPair;

/// Context information for a connection to an HTTP server.
typedef struct {
    /// A CivetWeb connection pointer.
    struct mg_connection *connection;
    /// The name of the server to connect to.
    const char *host;
    /// The port that the server is listening on.
    int port;
    /// The most recent status code.
    HttpStatus status_code;

    /// The timeout value to use when reading from the connection. (in milliseconds)
    int timeout;
} HttpConnection;

/// Create a value suitable for assigning to an HttpConnection structure.
#define HTTP_CONNECTION(HOST,PORT) (HttpConnection){ .host = (HOST), .port = (PORT), .timeout = HTTP_DEFAULT_TIMEOUT }

/// Send an HTTP request with an optional body and wait for a response from the
/// server. The headers and response code will be available if a response was
/// received.
/// @param[in,out] connection pointer to the connection information.
/// @param[in] action the kind of request to make: GET,POST,etc.
/// @param[in] uri the URI being requested.
/// @param[in] params optional pointer to a list of name/value pairs to encode
///          as part of the request. Use NULL if no extra parameters are needed.
///          The list should end with a pair that contains 2 NULLs.
/// @param[in] extraHeaders optional pointer to a list of name/value pairs to
///          use as extra headers in the request. For POST requests, the
///          content-length header is automatically included. Use NULL if no
///          extra headers are needed. The list should end with a pair that
///          contains 2 NULLs.
/// @return the http status code from the response.
HttpStatus beginHttpRequest(HttpConnection *connection, HttpAction action,
                            const char *uri, HttpPair *params,
                            HttpPair *extraHeaders, const char *body);

/// Get information about the response to the HTTP request. If the response code
/// is a success code, the body of the response will be returned as part of the
/// ResponseData. If there was a problem, the body of the response will be ignored.
/// @param[in,out] connection pointer to the connection information.
/// @param[in,out] response pointer to the structure where the response data and
///          size is to be stored.
/// @return the http status code from the response.
HttpStatus extractResponseBody(HttpConnection *connection, ResponseData *response);

/// Get binary blob data for the HTTP request from server up to 1MB size.
/// The actual size of data will be saved in response for each call.
/// Return the http status code from the response.
/// @param[in,out] connection pointer to the connection information.
/// @param[in,out] response pointer to the structure where the response data and
///          size is to be stored.
/// @return the http status code from the response.
HttpStatus extractBinaryResponseBody(HttpConnection *connection, ResponseData *response);

/// Return a pointer to the string value associated with a response header. NULL
/// will be returned if a problem was encountered or the header was not returned
/// as part of the response. The caller should use or copy the value before
/// calling another function in this module.
/// @param[in,out] connection pointer to the connection information.
/// @param[in] headerName the name of the header
/// @return pointer to the header's value.
const char *getResponseHeader(HttpConnection *connection, const char *headerName);

/// Close the socket associated with the connection. This must be done
/// explicitly so we can support doing multiple requests via the same
/// connection.
/// @param[in,out] connection pointer to the connection information.
void closeHttpConnection(HttpConnection *connection);

HttpStatus httpGetRequest (const char *hostname, const int port, const char *uri,
                        HttpPair *queryList, HttpPair *extraHeaders, ResponseData *rawData, const char *body);

HttpStatus httpPostRequest (const char *hostname, const int port, const char *uri,
                        HttpPair *extraHeaders, ResponseData *rawData,  const char *body);


#endif // SRC_HTTP_H
