#pragma once

#include "log_manager.h"

using std::wstring;
using std::wstring_view;
using std::wstringstream;
using std::experimental::filesystem::path;

using winrt::Windows::Networking::Sockets::StreamSocket;
using winrt::Windows::Networking::Sockets::StreamSocketListener;
using winrt::Windows::Storage::Streams::IInputStream;
using winrt::Windows::Storage::Streams::DataWriter;
using winrt::Windows::Foundation::IAsyncAction;

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

	http_server(const wchar_t* port_name, const wchar_t *server_name);
	IAsyncAction start();
	void on(const wchar_t* url, callback_t process) { _callbacks.emplace( url, process); }
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
	std::unordered_map<wstring, callback_t>	_callbacks;
};

template<class T> class rest_adapter {
public:
	using callback_t = http_server::callback_t;
	using content_t = http_server::content_t;
	using model = typename T::model;

	static callback_t get(T& db) {
		return [&db](auto&& req, auto&&) -> std::tuple<content_type, content_t> {
			switch(req.type) {
			case http_method::get: {
				auto pid = req.params.find(L"id");
				if(pid == req.params.end()) {
					return std::make_tuple(content_type::json, db.to_string());
				} else {
					return std::make_tuple(content_type::json, db.get(std::stoul(pid->second)).to_string());
				}
			}
			case http_method::post: {
				auto r = model{ JsonObject::Parse(req.body) };
				r.id = 0;
				auto id = db.save(r);
				logger.info(module_name, L"create rule %d", id);
				return std::make_tuple(content_type::json, db.get(id).to_string());
			}
			case http_method::put: {
				auto json = JsonObject::Parse(req.body);
				auto id = (unsigned)json.GetNamedNumber(L"id", 0);
				auto r = db.get(id);
				if(!r.id)		throw http_status::not_found;
				r.update(json);
				db.save(r);
				logger.info(module_name, L"update rule %d", id);
				return std::make_tuple(content_type::json, db.get(id).to_string());
			}
			case http_method::del: {
				auto id = std::stoul(req.params[L"id"s]);
				db.remove(id);
				logger.info(module_name, L"delete rule %d", id);
				return std::make_tuple(content_type::text, L"");
			}
			default: throw http_status::method_not_allowed;
			}
		};
	}
};
