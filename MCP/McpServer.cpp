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

#include "MCP/McpServer.h"
#include "GdaJson.h"
#include "GeoDa.h"

namespace
{
    json_spirit::Pair P(const wxString& name, const json_spirit::Value& v)
    {
        return json_spirit::Pair(name.ToStdString(), v);
    }

    json_spirit::Value Obj(const std::vector<json_spirit::Pair>& pairs)
    {
        return json_spirit::Value(json_spirit::Object(pairs));
    }

    // Extract the "id" member of a JSON-RPC request (number, string, or null).
    json_spirit::Value GetRequestId(const json_spirit::Object& obj)
    {
        json_spirit::Value id;
        if (GdaJson::findValue(obj, id, "id")) return id;
        return json_spirit::Value();
    }
}

McpServer::McpServer()
{
}

McpServer::~McpServer()
{
}

json_spirit::Value McpServer::MakeParseError() const
{
    std::vector<json_spirit::Pair> err;
    err.push_back(P("code", json_spirit::Value(-32700)));
    err.push_back(P("message", json_spirit::Value("Parse error")));
    std::vector<json_spirit::Pair> resp;
    resp.push_back(P("jsonrpc", json_spirit::Value("2.0")));
    resp.push_back(P("id", json_spirit::Value()));
    resp.push_back(P("error", Obj(err)));
    return Obj(resp);
}

bool McpServer::IsHeavyTool(const json_spirit::Value& request) const
{
    if (request.type() != json_spirit::obj_type) return false;
    const json_spirit::Object& obj = request.get_obj();
    wxString method = GdaJson::getStrValFromObj(obj, "method");
    if (method != "tools/call") return false;
    json_spirit::Value params;
    if (!GdaJson::findValue(request, params, "params")) return false;
    if (params.type() != json_spirit::obj_type) return false;
    wxString name = GdaJson::getStrValFromObj(params.get_obj(), "name");
    const McpTool* tool = m_tools.FindTool(name);
    return tool && tool->run_on_worker;
}

json_spirit::Value McpServer::HandleRequest(const std::string& body)
{
    json_spirit::Value request;
    if (!json_spirit::read(body, request)) {
        return MakeParseError();
    }
    return Dispatch(request);
}

json_spirit::Value McpServer::Dispatch(const json_spirit::Value& request)
{
    if (request.type() != json_spirit::obj_type) {
        std::vector<json_spirit::Pair> err;
        err.push_back(P("code", json_spirit::Value(-32600)));
        err.push_back(P("message",
                        json_spirit::Value("Invalid Request: expected an object")));
        std::vector<json_spirit::Pair> resp;
        resp.push_back(P("jsonrpc", json_spirit::Value("2.0")));
        resp.push_back(P("id", json_spirit::Value()));
        resp.push_back(P("error", Obj(err)));
        return Obj(resp);
    }

    const json_spirit::Object& obj = request.get_obj();
    bool is_notification = !GdaJson::hasName(obj, "id");
    json_spirit::Value id = GetRequestId(obj);
    wxString method = GdaJson::getStrValFromObj(obj, "method");

    json_spirit::Value params;
    GdaJson::findValue(request, params, "params");
    json_spirit::Object params_obj;
    if (params.type() == json_spirit::obj_type) params_obj = params.get_obj();

    json_spirit::Value inner;
    bool is_error = false;
    int error_code = 0;
    wxString error_message;

    if (method == "initialize") {
        inner = HandleInitialize(params_obj);
    } else if (method == "notifications/initialized") {
        inner = Obj(std::vector<json_spirit::Pair>());
    } else if (method == "ping") {
        inner = Obj(std::vector<json_spirit::Pair>());
    } else if (method == "tools/list") {
        inner = HandleToolsList();
    } else if (method == "tools/call" || method == "prompts/get" ||
               method == "resources/read") {
        try {
            if (method == "tools/call") {
                inner = HandleToolsCall(params_obj);
            } else if (method == "prompts/get") {
                inner = HandlePromptsGet(params_obj);
            } else {
                inner = HandleResourcesRead(params_obj);
            }
        } catch (const McpError& e) {
            is_error = true;
            error_code = e.code();
            error_message = e.message();
        } catch (const std::exception& e) {
            is_error = true;
            error_code = -32603;
            error_message = e.what();
        }
    } else if (method == "prompts/list") {
        inner = HandlePromptsList();
    } else if (method == "resources/list") {
        inner = HandleResourcesList();
    } else {
        is_error = true;
        error_code = -32601;
        error_message = "Method not found: " + method;
    }

    // Notifications get no response.
    if (is_notification) return json_spirit::Value();

    std::vector<json_spirit::Pair> resp;
    resp.push_back(P("jsonrpc", json_spirit::Value("2.0")));
    resp.push_back(P("id", id));
    if (is_error) {
        std::vector<json_spirit::Pair> err;
        err.push_back(P("code", json_spirit::Value(error_code)));
        err.push_back(P("message", json_spirit::Value(error_message.ToStdString())));
        resp.push_back(P("error", Obj(err)));
    } else {
        resp.push_back(P("result", inner));
    }
    return Obj(resp);
}

json_spirit::Value McpServer::HandleInitialize(const json_spirit::Object& params)
{
    std::vector<json_spirit::Pair> tools_cap;
    tools_cap.push_back(P("listChanged", json_spirit::Value(false)));
    std::vector<json_spirit::Pair> prompts_cap;
    std::vector<json_spirit::Pair> resources_cap;
    resources_cap.push_back(P("listChanged", json_spirit::Value(false)));
    std::vector<json_spirit::Pair> capabilities;
    capabilities.push_back(P("tools", Obj(tools_cap)));
    capabilities.push_back(P("prompts", Obj(prompts_cap)));
    capabilities.push_back(P("resources", Obj(resources_cap)));

    std::vector<json_spirit::Pair> server_info;
    server_info.push_back(P("name", json_spirit::Value("geoda-mcp")));
    server_info.push_back(P("version", json_spirit::Value("1.0.0")));

    std::vector<json_spirit::Pair> result;
    result.push_back(P("protocolVersion", json_spirit::Value("2025-03-26")));
    result.push_back(P("capabilities", Obj(capabilities)));
    result.push_back(P("serverInfo", Obj(server_info)));
    return Obj(result);
}

json_spirit::Value McpServer::HandleToolsList()
{
    return m_tools.GetToolsList();
}

json_spirit::Value McpServer::HandlePromptsList()
{
    return m_prompts.GetPromptsList();
}

json_spirit::Value McpServer::HandlePromptsGet(const json_spirit::Object& params)
{
    return m_prompts.GetPrompt(params);
}

json_spirit::Value McpServer::HandleResourcesList()
{
    return m_resources.GetResourcesList();
}

json_spirit::Value McpServer::HandleResourcesRead(const json_spirit::Object& params)
{
    return m_resources.GetResource(params);
}

json_spirit::Value McpServer::HandleToolsCall(const json_spirit::Object& params)
{
    wxString name = GdaJson::getStrValFromObj(params, "name");
    const McpTool* tool = m_tools.FindTool(name);
    if (!tool) {
        throw McpError(-32602, "Unknown tool: " + name.ToStdString());
    }

    McpToolContext ctx;
    ctx.project = GdaFrame::GetProject();

    json_spirit::Object args;
    json_spirit::Value args_val;
    if (GdaJson::findValue(params, args_val, "arguments") &&
        args_val.type() == json_spirit::obj_type) {
        args = args_val.get_obj();
    }

    json_spirit::Value result = tool->handler(ctx, args);

    // If the handler returned a MCP "content" array directly (e.g. an image
    // snapshot from return_image), pass it through verbatim. Otherwise wrap
    // the JSON result in a single text block (default behavior).
    json_spirit::Array content;
    bool has_content = false;
    if (result.type() == json_spirit::obj_type) {
        const json_spirit::Object& o = result.get_obj();
        for (json_spirit::Object::const_iterator it = o.begin();
             it != o.end(); ++it) {
            if (it->name_ == "content" &&
                it->value_.type() == json_spirit::array_type) {
                content = it->value_.get_array();
                has_content = true;
                break;
            }
        }
    }
    if (!has_content) {
        std::vector<json_spirit::Pair> content_item;
        content_item.push_back(P("type", json_spirit::Value("text")));
        content_item.push_back(P("text",
            json_spirit::Value(wxString::FromUTF8(json_spirit::write(result).c_str())
                               .ToStdString())));
        content.push_back(Obj(content_item));
    }
    std::vector<json_spirit::Pair> call_result;
    call_result.push_back(P("content", json_spirit::Value(content)));
    return Obj(call_result);
}
