#pragma once

using std::wstring;
using std::wstring_view;
using std::wstringstream;
using std::experimental::filesystem::path;

using winrt::Windows::Networking::Sockets::StreamSocket;
using winrt::Windows::Networking::Sockets::StreamSocketListener;
using winrt::Windows::Storage::Streams::IInputStream;
using winrt::Windows::Storage::Streams::DataWriter;

enum class http_status {
	ok,
	found,
	unauthorized,
	forbidden,
	not_found,
	method_not_allowed,
	unsuported_media,
	internal_server_error
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
};

struct http_request
{
	wstring type;
	std::map<wstring, wstring> params;
	std::map<wstring, wstring> attribs;
	wstring path;
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
	using process_fun_t = std::function<std::tuple<content_type, content_t>(http_request&, http_response&)>;
	using action_fun_t = std::function<void(http_request&, const wstring& value)>;

	http_server(const wchar_t* port_name, const wchar_t *server_name);
	std::future<void> start();
	void add(const wchar_t* url, process_fun_t callback) { _callbacks.emplace( url, callback); }
	void add(const wchar_t* url, path file_name);
	void add_action(const wchar_t* action, action_fun_t callback) { _actions.emplace( action, callback ); }
private:
	size_t read_request(const IInputStream& stream, wstring& content);
	void parse_request(wstring_view content, http_request& request);
	void write_response(DataWriter& writer, const http_response& response);
	void write_content(DataWriter& writer, const content_t& content);
	std::future<void> on_connection(StreamSocket socket);
	
	wstring _port;
	wstring _server_name;
	StreamSocketListener _listener;
	std::unordered_map<wstring, content_type>	_content_types;
	std::unordered_map<wstring, process_fun_t>	_callbacks;
	std::unordered_map<wstring, action_fun_t>	_actions;
};

