#pragma once

#include "log_manager.h"

using std::wstring;
using std::wstring_view;
using std::wstringstream;
using std::experimental::filesystem::path;
using concurrency::task;

using winrt::Windows::Networking::Sockets::StreamSocket;
using winrt::Windows::Networking::Sockets::StreamSocketListener;
using winrt::Windows::Storage::Streams::IInputStream;
using winrt::Windows::Storage::Streams::DataWriter;

enum class http_status {
	ok,
	created,
	accepted,
	no_content,
	found,
	unauthorized,
	forbidden,
	not_found,
	method_not_allowed,
	unsuported_media,
	internal_server_error
};

enum class http_method {
	get,
	post,
	put,
	del,
};

enum class content_type {
	html,
	xml,
	css,
	json,
	javascript,
	jpg,
	png,
	gif,
	ico,
	pdf,
	zip,
	gz,
	text,
	none
};

struct http_request
{
	http_method type;
	std::map<wstring, wstring> params;
	std::map<wstring, wstring> attribs;
	wstring path;
	wstring rest;
	wstring body;
};

struct http_response
{
	http_status status;
	content_type content_type;
	uint32_t content_size;
	wstring params;
};

class http_server
{
public:
	using binary_t = std::vector<uint8_t>;
	using content_t = std::variant<wstring, binary_t>;
	using callback_t = std::function<std::tuple<content_type, content_t>(http_request&, http_response&)>;
	using callback_entry_t = std::pair<wstring, callback_t>;

	http_server(const wchar_t* port_name, const wchar_t *server_name);
	task<void> start();
	void on(const wchar_t* url, callback_t process) { _callbacks.emplace_back(std::make_pair(url, process)); }
	void on(const wchar_t* url, path file_name);
private:
	size_t read_request(const IInputStream& stream, wstring& content);
	void parse_request(wstring_view content, http_request& request);
	void write_response(DataWriter& writer, const http_response& response);
	void write_content(DataWriter& writer, const content_t& content);
	winrt::fire_and_forget on_connection(StreamSocket socket);
	
	wstring				_port;
	wstring				_server_name;
	StreamSocketListener _listener;
	
	std::unordered_map<wstring, content_type>	_content_types;
	std::vector<callback_entry_t>				_callbacks;
};

template<class T> class rest_adapter {
public:
	using callback_t = http_server::callback_t;
	using content_t = http_server::content_t;

	static callback_t get(T& db) {
		return [&db](auto&& req, auto&& resp) -> std::tuple<content_type, content_t> {
			auto id = req.rest.empty() ? 0 : std::stoul(req.rest.c_str() + 1);
			switch(req.type) {
			case http_method::get:
				return std::make_tuple(content_type::json, id ? db.get(id).to_string() : db.to_string());
			case http_method::post: {
				if(req.body.empty())	throw http_status::unsuported_media;
				auto new_id = db.save({ JsonObject::Parse(req.body) });
				resp.params = L"Location: " + req.path + L"/" + std::to_wstring(new_id) + L"\r\n";
				resp.status = http_status::created;
				return std::make_tuple(content_type::text, L"");
			}
			case http_method::put: {
				auto obj = db.get(id);
				if(!obj.id)				throw http_status::not_found;
				if(req.body.empty())	throw http_status::unsuported_media;
				obj = JsonObject::Parse(req.body);
				obj.id = id;
				db.save(obj);
				return std::make_tuple(content_type::text, L"");
			}
			case http_method::del: {
				db.remove(id);
				resp.status = http_status::no_content;
				return std::make_tuple(content_type::text, L"");
			}
			default: throw http_status::method_not_allowed;
			}
		};
	}
};
