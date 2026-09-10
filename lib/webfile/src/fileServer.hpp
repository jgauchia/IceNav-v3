/**
 * @file fileServer.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Layer-1 file server interface
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

/**
 * @class IFileServer
 * @brief Layer-1 contract for the SD file server over HTTP.
 *
 * @details Serving files is a service that rides on top of an active radio link
 *          (see IConnectivity) but is not itself a radio transport, so it lives
 *          in its own contract. start() brings up the HTTP server, stop() tears
 *          it down and process() runs the deferred tasks from the main loop.
 */
class IFileServer
{
public:
    virtual ~IFileServer() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void process() = 0;
};

IFileServer &fileServer();
