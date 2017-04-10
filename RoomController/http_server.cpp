#include "pch.h"
#include "common.h"
#include "http_server.h"

using namespace std::string_literals;
using namespace winrt::Windows::Storage::Streams;

using winrt::Windows::Foundation::WwwFormUrlDecoder;

const wchar_t module_name[] = L"HTTP";

const wchar_t *status_codes[] = {
	L"200 OK",
	L"302 Found",
	L"401 Unauthorized",
	L"403 Forbidden",
	L"404 Not Found",
	L"405 Method not allowed",
	L"415 Unsupported Media Type",
	L"500 Internal Server Error"
};

const wchar_t *content_types[] = {
	L"text/html",
	L"text/xml",
	L"text/css",
	L"application/json",
	L"application/javascript",
	L"image/jpeg",
	L"image/png",
	L"image/gif",
	L"image/x-icon",
	L"application/x-pdf",
	L"application/x-zip",
	L"application/x-gzip",
	L"text/plain",
};

wstring_view trim(wstring_view str, wchar_t trim_char = ' ')
{
	while(!str.empty() && str.front() == trim_char)	str.remove_prefix(1);
	while(!str.empty() && str.back() == trim_char)	str.remove_suffix(1);
	return str;
}

std::vector<wstring_view> split(wstring_view src, wchar_t separator = ' ')
{
	std::vector<wstring_view> result;
	size_t start = 0, end;
	while((end = src.find(separator, start)) != wstring_view::npos) {
		result.push_back(src.substr(start, end - start));
		start = end + 1;
	}
	result.push_back(src.substr(start));
	return result;
}

http_response http_error(http_status status, http_server::content_t& content)
{
	auto ps = std::get_if<wstring>(&content);
	if(ps)	content = L"<HTML><body>"s + status_codes[(int)status] + L"<br>"s + *ps + L"</body></HTML>"s;
	return http_response{ status, content_type::html };
}

http_server::http_server(const wchar_t* port_name, const wchar_t* server_name, log_manager& log) : 
	_port(port_name), _server_name(server_name), _log(log)
{
	_content_types.emplace(L".htm"s,	content_type::html);
	_content_types.emplace(L".html"s,	content_type::html);
	_content_types.emplace(L".xml"s,	content_type::xml);
	_content_types.emplace(L".css"s,	content_type::css);
	_content_types.emplace(L".jpg"s,	content_type::jpg);
	_content_types.emplace(L".jpeg"s,	content_type::jpg);
	_content_types.emplace(L".gif"s,	content_type::gif);
	_content_types.emplace(L".png"s,	content_type::png);
	_content_types.emplace(L".ico"s,	content_type::ico);
	_content_types.emplace(L".js"s,		content_type::javascript);
	_content_types.emplace(L".json"s,	content_type::json);
	_content_types.emplace(L".pdf"s,	content_type::pdf);
	_content_types.emplace(L".zip"s,	content_type::zip);
	_content_types.emplace(L".gz"s,		content_type::gz);
}

std::future<void> http_server::start()
{
	_listener.ConnectionReceived([this](auto&&, auto&& args) { on_connection(args.Socket()); });
	co_await _listener.BindServiceNameAsync(_port);
}

size_t http_server::read_request(const IInputStream& stream, wstring& content)
{
	std::array<uint8_t, 4096> buffer;
	DataWriter writer;
	writer.WriteBytes(buffer);
	auto pbuf = writer.DetachBuffer();

	size_t dataRead = buffer.size();

	while (dataRead == buffer.size()) {
		auto res = stream.ReadAsync(pbuf, pbuf.Capacity(), InputStreamOptions::Partial).get();
		content += DataReader::FromBuffer(res).ReadString(res.Length());
		dataRead = res.Length();
	}
	return content.length();
}
void http_server::parse_request(wstring_view content, http_request& request)
{
	auto rows = split(content, '\n');
	if(rows.size() < 1)		throw http_status::internal_server_error;

	// header
	auto header = split(rows[0], ' ');
	if(header.size() < 2)	throw http_status::internal_server_error;
	request.type = header[0];
	auto path = split(header[1], '?');
	if(path.size() < 1)		throw http_status::internal_server_error;
	request.path = path[0];

	// GET parameters
	if(path.size() > 1) {
		WwwFormUrlDecoder params{ wstring(path[1]) };
		for(auto p : params)	request.params.emplace(p.Name(), p.Value());
	}

	// attributes
	request.attribs.clear();
	size_t i;
	for(i = 1; i < rows.size(); i++) {
		if(rows[i] == L"\r")	break;
		auto attr = split(trim(rows[i], '\r'), ':');
		if(attr.size() == 2)	request.attribs.emplace(attr[0], trim(attr[1]));
	}

	// POST parameters
	if(request.type == L"POST" && i+1 < rows.size()) {
		if(request.attribs[L"Content-Type"] != L"application/x-www-form-urlencoded")	throw http_status::unsuported_media;
		WwwFormUrlDecoder params{ wstring(rows[i + 1]) };
		for(auto p : params)	request.params.emplace(p.Name(), p.Value());
	}
}

DataWriter& operator << (DataWriter& w, const wstring& s) { return w.WriteString(s), w; }
DataWriter& operator << (DataWriter& w, const wchar_t* s) { return w.WriteString(s), w; }

void http_server::write_response(DataWriter& writer, const http_response& response)
{
	const wchar_t eol[] = L"\r\n";
	writer << L"HTTP/1.1 " << status_codes[(unsigned)response.status] << eol;
	writer << L"Server: " << _server_name << eol;
	if (response.content_size > 0) {
		writer << L"Content-Type: " << content_types[(unsigned)response.content_type] << eol;
		writer << L"Content-Length: " << std::to_wstring(response.content_size) << eol;
	}
	writer << L"Connection: close" << eol;
	writer << response.params;
}

void http_server::write_content(DataWriter& writer, const content_t& content)
{
	writer.WriteString(L"\r\n");
	struct visitor {
		DataWriter& writer;
		void operator()(const wstring& s)				{ writer.WriteString(s); };
		void operator()(const std::vector<uint8_t> v)	{ writer.WriteBytes(v);};
	} v { writer };
	std::visit(v, content);
}

void http_server::add(const wchar_t* url, path file_name)
{
	auto pct = _content_types.find(file_name.extension());
	if (pct == _content_types.end())	throw std::invalid_argument("Unknow file type");
	add(url, [type = pct->second, file_name](const http_request& request, http_response& response) {
		size_t size = (size_t)std::experimental::filesystem::file_size(file_name);
		std::ifstream ifs(file_name, std::ios::binary);
		content_t content( std::in_place_type<binary_t>, size);
		ifs.read((char *)std::get<binary_t>(content).data(), size);
		return std::make_tuple(type, content);
	});
}

std::future<void> http_server::on_connection(StreamSocket socket)
{
	try {
		co_await winrt::resume_background();
		content_t answer;
		wstring content;
		http_request req;
		http_response resp;
		try {
			resp.status = http_status::ok;
			if (!read_request(socket.InputStream(), content))	co_return;
			parse_request(content, req);

			_log.message(module_name, L"%s %s", req.type.c_str(), req.path.c_str());

			for(auto p : req.params) {
				auto pc = _actions.find(p.first);
				if (pc != _actions.end()) {
					(pc->second)(req, p.second);
					resp.status = http_status::found;
					resp.params = L"Location: " + req.path + L"\r\n";
				}
			}
			
			if (resp.status != http_status::found) {
				auto pc = _callbacks.find(req.path);
				if (pc == _callbacks.end()) throw http_status::not_found;
				std::tie(resp.content_type, answer) = pc->second(req, resp);
			}

		} catch(http_status status) {
			resp = http_error(status, answer);
		}

		DataWriter writer(socket.OutputStream());
		writer.UnicodeEncoding(UnicodeEncoding::Utf8);
		resp.content_size = answer.index() == 0 ? writer.MeasureString(std::get<wstring>(answer)) :
												  std::get<binary_t>(answer).size();
		write_response(writer, resp);
		if(resp.content_size > 0)	write_content(writer, answer);
		co_await writer.StoreAsync();
		co_return;
	} catch(winrt::hresult_error& hr) {
		_log.error(module_name, hr);
	} catch(std::exception& ex) {
		_log.error(module_name, ex);
	}	catch (...) {
		_log.error(module_name, L"unknown error");
	}
}

