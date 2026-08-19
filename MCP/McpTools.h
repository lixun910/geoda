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

#ifndef __GEODA_CENTER_MCP_TOOLS_H__
#define __GEODA_CENTER_MCP_TOOLS_H__

#include <exception>
#include <string>
#include <vector>
#include <json_spirit/json_spirit.h>
#include <wx/string.h>

class Project;

// Context passed to every tool handler. project is null when no project is
// open.
struct McpToolContext
{
    Project* project;
};

// Thrown by tool handlers for expected errors (no project open, unknown
// column, unknown weights id, ...). McpServer converts these to JSON-RPC
// errors.
class McpError : public std::exception
{
public:
    McpError(int code, const std::string& message)
        : m_code(code), m_message(message) {}
    virtual ~McpError() throw() {}
    virtual const char* what() const throw() { return m_message.c_str(); }
    int code() const { return m_code; }
    const std::string& message() const { return m_message; }

private:
    int m_code;
    std::string m_message;
};

// A tool handler takes the tool context and the JSON-RPC "arguments" object
// and returns the tool's result as a json_spirit::Value. Expected errors are
// signaled by throwing McpError.
typedef json_spirit::Value (*McpToolHandler)(const McpToolContext& ctx,
                                             const json_spirit::Object& params);

struct McpTool
{
    wxString name;                    // command id (MCP tool name)
    wxString label;                   // human-readable label
    wxString menu_path;               // menu grouping, e.g. "Space"
    wxString description;
    json_spirit::Value input_schema;  // JSON Schema object
    bool run_on_worker;               // heavy tools run on a worker thread
    McpToolHandler handler;
};

// Registry of MCP tools. Populated once in the constructor; read-only
// afterwards so it is safe to query from worker threads.
class McpTools
{
public:
    McpTools();
    ~McpTools();

    const McpTool* FindTool(const wxString& name) const;
    const std::vector<McpTool>& GetTools() const { return m_tools; }
    json_spirit::Value GetToolsList() const;

    // Registration API, used by RegisterCommands (MCP/McpCommands.cpp).
    void AddTool(const wxString& command_id, const wxString& label,
                 const wxString& menu_path, const wxString& description,
                 const json_spirit::Value& input_schema, bool run_on_worker,
                 McpToolHandler handler);

private:
    void RegisterTools();

    std::vector<McpTool> m_tools;
};

#endif
