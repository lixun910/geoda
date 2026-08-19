/**
 * GeoDa TM, Copyright (C) 2011-2025 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEODA_CENTER_MCP_SERVER_H__
#define __GEODA_CENTER_MCP_SERVER_H__

#include <string>
#include <json_spirit/json_spirit.h>
#include <wx/string.h>
#include "MCP/McpTools.h"
#include "MCP/McpPrompts.h"
#include "MCP/McpResources.h"

// JSON-RPC 2.0 dispatch for the MCP protocol. Stateless: each request is
// handled independently. HandleRequest may be called from the main thread
// (light and window tools) or from a worker thread (heavy tools); the tool
// registry is read-only after construction.
class McpServer
{
public:
    McpServer();
    ~McpServer();

    // Parse and handle a JSON-RPC request body. Returns the JSON-RPC response
    // as a Value. For notifications (no "id"), returns an empty Value and no
    // response should be sent.
    json_spirit::Value HandleRequest(const std::string& body);

    // True if the request is a tools/call for a heavy tool that should run on
    // a worker thread.
    bool IsHeavyTool(const json_spirit::Value& request) const;

    // JSON-RPC parse error response (used by the HTTP layer when the body is
    // not valid JSON).
    json_spirit::Value MakeParseError() const;

private:
    json_spirit::Value Dispatch(const json_spirit::Value& request);
    json_spirit::Value HandleInitialize(const json_spirit::Object& params);
    json_spirit::Value HandleToolsList();
    json_spirit::Value HandleToolsCall(const json_spirit::Object& params);
    json_spirit::Value HandlePromptsList();
    json_spirit::Value HandlePromptsGet(const json_spirit::Object& params);
    json_spirit::Value HandleResourcesList();
    json_spirit::Value HandleResourcesRead(const json_spirit::Object& params);

    McpTools m_tools;
    McpPrompts m_prompts;
    McpResources m_resources;
};

#endif
