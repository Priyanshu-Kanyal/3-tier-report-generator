/**
 * High-Performance C++ Report Rendering Service
 * Listens on port 8081 for POST /render requests.
 * Renders dynamic PDF and Excel/CSV reports based on template schema & parameters.
 */

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <map>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef int socklen_t;
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  #define closesocket close
  #define SOCKET int
  #define INVALID_SOCKET -1
  #define SOCKET_ERROR -1
#endif

// Lightweight JSON field extraction helpers
std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos);
    if (pos == std::string::npos) return "";
    size_t end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

std::vector<std::string> extractJsonStringArray(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return result;
    pos = json.find('[', pos);
    if (pos == std::string::npos) return result;
    size_t end = json.find(']', pos);
    if (end == std::string::npos) return result;
    
    std::string arrContent = json.substr(pos + 1, end - pos - 1);
    size_t p = 0;
    while ((p = arrContent.find('"', p)) != std::string::npos) {
        size_t q = arrContent.find('"', p + 1);
        if (q == std::string::npos) break;
        result.push_back(arrContent.substr(p + 1, q - p - 1));
        p = q + 1;
    }
    return result;
}

std::map<std::string, std::string> extractParameters(const std::string& json) {
    std::map<std::string, std::string> params;
    size_t pos = json.find("\"parameters\"");
    if (pos == std::string::npos) return params;
    pos = json.find('{', pos);
    if (pos == std::string::npos) return params;
    
    // Simple bracket match
    int depth = 1;
    size_t end = pos + 1;
    while (end < json.size() && depth > 0) {
        if (json[end] == '{') depth++;
        else if (json[end] == '}') depth--;
        end++;
    }
    std::string body = json.substr(pos + 1, end - pos - 2);
    
    size_t p = 0;
    while (p < body.size()) {
        size_t kStart = body.find('"', p);
        if (kStart == std::string::npos) break;
        size_t kEnd = body.find('"', kStart + 1);
        if (kEnd == std::string::npos) break;
        std::string key = body.substr(kStart + 1, kEnd - kStart - 1);
        
        size_t col = body.find(':', kEnd);
        if (col == std::string::npos) break;
        
        // Find value (could be string or number)
        size_t vStart = body.find_first_not_of(" \t\r\n", col + 1);
        if (vStart == std::string::npos) break;
        
        std::string val;
        if (body[vStart] == '"') {
            size_t vEnd = body.find('"', vStart + 1);
            if (vEnd == std::string::npos) break;
            val = body.substr(vStart + 1, vEnd - vStart - 1);
            p = vEnd + 1;
        } else {
            size_t vEnd = body.find_first_of(",}\r\n", vStart);
            if (vEnd == std::string::npos) vEnd = body.size();
            val = body.substr(vStart, vEnd - vStart);
            p = vEnd;
        }
        params[key] = val;
    }
    return params;
}

// Generate valid PDF document conforming to PDF 1.4 spec
std::string renderPdf(const std::string& title, 
                      const std::map<std::string, std::string>& params, 
                      const std::vector<std::string>& columns) {
    std::ostringstream streamObj;
    
    // Current timestamp
    std::time_t now = std::time(nullptr);
    char dateBuf[64];
    std::strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M:%S UTC", std::gmtime(&now));

    // Construct PDF content stream
    streamObj << "BT\n";
    // Title
    streamObj << "/F1 22 Tf\n";
    streamObj << "50 740 Td\n";
    streamObj << "(" << (title.empty() ? "Generated Report" : title) << ") Tj\n";
    streamObj << "ET\n";

    // Subtitle & Metadata
    streamObj << "BT\n";
    streamObj << "/F2 10 Tf\n";
    streamObj << "50 715 Td\n";
    streamObj << "(Generated on: " << dateBuf << " | Engine: C++ Native Renderer 1.0) Tj\n";
    streamObj << "ET\n";

    // Divider line
    streamObj << "q\n";
    streamObj << "0.2 0.3 0.6 rg\n";
    streamObj << "50 705 512 2 re f\n";
    streamObj << "Q\n";

    // Parameters Box
    streamObj << "q\n";
    streamObj << "0.95 0.96 0.98 rg\n";
    streamObj << "50 630 512 65 re f\n";
    streamObj << "0.8 0.85 0.9 RG\n";
    streamObj << "50 630 512 65 re S\n";
    streamObj << "Q\n";

    streamObj << "BT\n";
    streamObj << "/F1 11 Tf\n";
    streamObj << "60 675 Td\n";
    streamObj << "(REPORT PARAMETERS) Tj\n";
    streamObj << "ET\n";

    int paramY = 655;
    for (const auto& kv : params) {
        streamObj << "BT\n";
        streamObj << "/F2 10 Tf\n";
        streamObj << "60 " << paramY << " Td\n";
        streamObj << "(" << kv.first << ": " << kv.second << ") Tj\n";
        streamObj << "ET\n";
        paramY -= 15;
    }

    // Table Header Background
    streamObj << "q\n";
    streamObj << "0.15 0.25 0.5 rg\n";
    streamObj << "50 590 512 24 re f\n";
    streamObj << "Q\n";

    // Table Column Headers
    int colX = 60;
    int colWidth = columns.empty() ? 100 : (490 / std::max((int)columns.size(), 1));
    for (const auto& col : columns) {
        streamObj << "BT\n";
        streamObj << "/F1 10 Tf\n";
        streamObj << "1 1 1 rg\n"; // White text
        streamObj << colX << " 598 Td\n";
        
        std::string headerName = col;
        std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::toupper);
        streamObj << "(" << headerName << ") Tj\n";
        streamObj << "ET\n";
        colX += colWidth;
    }

    // Sample data rows matching template columns
    std::vector<std::vector<std::string>> rows;
    if (title.find("Sales") != std::string::npos) {
        rows = {
            {"2026-01-05", "Enterprise Cloud Suite", "45", "$4,500.00"},
            {"2026-01-12", "Analytics Pro License", "120", "$14,400.00"},
            {"2026-01-18", "Data Pipeline Connector", "15", "$3,750.00"},
            {"2026-01-22", "High-Availability Addon", "8", "$8,000.00"},
            {"2026-01-28", "Developer API Key Pack", "32", "$9,600.00"},
            {"2026-01-30", "Enterprise Support SLA", "10", "$15,000.00"}
        };
    } else {
        rows = {
            {"SKU-1001", "Hydraulic Actuator Pump", "184", "50 units"},
            {"SKU-1002", "Titanium Fastener 10mm", "2,450", "500 units"},
            {"SKU-1003", "Precision Sensor Module", "42", "60 units"},
            {"SKU-1004", "Modular Edge Gateway", "12", "15 units"},
            {"SKU-1005", "Lithium Backup Cell 48V", "95", "30 units"}
        };
    }

    int rowY = 565;
    bool alt = false;
    for (const auto& row : rows) {
        if (alt) {
            streamObj << "q\n";
            streamObj << "0.96 0.97 0.98 rg\n";
            streamObj << "50 " << (rowY - 5) << " 512 20 re f\n";
            streamObj << "Q\n";
        }
        alt = !alt;

        int cellX = 60;
        for (size_t c = 0; c < row.size() && c < columns.size(); ++c) {
            streamObj << "BT\n";
            streamObj << "/F2 9 Tf\n";
            streamObj << "0.1 0.1 0.1 rg\n";
            streamObj << cellX << " " << rowY << " Td\n";
            streamObj << "(" << row[c] << ") Tj\n";
            streamObj << "ET\n";
            cellX += colWidth;
        }
        rowY -= 22;
    }

    // Footer
    streamObj << "BT\n";
    streamObj << "/F2 8 Tf\n";
    streamObj << "0.5 0.5 0.5 rg\n";
    streamObj << "50 50 Td\n";
    streamObj << "(Report Generator 3-Tier Demo - Native C++ Backend Service) Tj\n";
    streamObj << "ET\n";

    std::string streamContent = streamObj.str();

    // Assemble PDF Object Structure
    std::ostringstream pdf;
    std::vector<size_t> xref;
    pdf << "%PDF-1.4\n%\xe2\xe3\xcf\xd3\n";

    // 1: Catalog
    xref.push_back(pdf.tellp());
    pdf << "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";

    // 2: Pages
    xref.push_back(pdf.tellp());
    pdf << "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";

    // 3: Page
    xref.push_back(pdf.tellp());
    pdf << "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
        << "/Contents 4 0 R /Resources << /Font << /F1 5 0 R /F2 6 0 R >> >> >>\nendobj\n";

    // 4: Content Stream
    xref.push_back(pdf.tellp());
    pdf << "4 0 obj\n<< /Length " << streamContent.length() << " >>\nstream\n"
        << streamContent
        << "endstream\nendobj\n";

    // 5: Font F1 (Helvetica-Bold)
    xref.push_back(pdf.tellp());
    pdf << "5 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold >>\nendobj\n";

    // 6: Font F2 (Helvetica)
    xref.push_back(pdf.tellp());
    pdf << "6 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>\nendobj\n";

    // Cross-reference table
    size_t xrefPos = pdf.tellp();
    pdf << "xref\n0 7\n";
    pdf << "0000000000 65535 f \n";
    for (size_t offset : xref) {
        pdf << std::setw(10) << std::setfill('0') << offset << " 00000 n \n";
    }

    // Trailer
    pdf << "trailer\n<< /Size 7 /Root 1 0 R >>\n";
    pdf << "startxref\n" << xrefPos << "\n%%EOF\n";

    return pdf.str();
}

// Generate Excel (CSV / Spreadsheet XML)
std::string renderExcel(const std::string& title, 
                        const std::map<std::string, std::string>& params, 
                        const std::vector<std::string>& columns) {
    std::ostringstream csv;
    csv << "# " << (title.empty() ? "Generated Report" : title) << "\r\n";
    csv << "# Filter Parameters:\r\n";
    for (const auto& kv : params) {
        csv << "#   " << kv.first << ": " << kv.second << "\r\n";
    }
    csv << "\r\n";

    // Header
    for (size_t i = 0; i < columns.size(); ++i) {
        csv << "\"" << columns[i] << "\"";
        if (i + 1 < columns.size()) csv << ",";
    }
    csv << "\r\n";

    // Sample data rows
    if (title.find("Sales") != std::string::npos) {
        csv << "\"2026-01-05\",\"Enterprise Cloud Suite\",45,4500.00\r\n";
        csv << "\"2026-01-12\",\"Analytics Pro License\",120,14400.00\r\n";
        csv << "\"2026-01-18\",\"Data Pipeline Connector\",15,3750.00\r\n";
        csv << "\"2026-01-22\",\"High-Availability Addon\",8,8000.00\r\n";
        csv << "\"2026-01-28\",\"Developer API Key Pack\",32,9600.00\r\n";
        csv << "\"2026-01-30\",\"Enterprise Support SLA\",10,15000.00\r\n";
    } else {
        csv << "\"SKU-1001\",\"Hydraulic Actuator Pump\",184,50\r\n";
        csv << "\"SKU-1002\",\"Titanium Fastener 10mm\",2450,500\r\n";
        csv << "\"SKU-1003\",\"Precision Sensor Module\",42,60\r\n";
        csv << "\"SKU-1004\",\"Modular Edge Gateway\",12,15\r\n";
        csv << "\"SKU-1005\",\"Lithium Backup Cell 48V\",95,30\r\n";
    }

    return csv.str();
}

void handleClient(SOCKET clientSocket) {
    char buffer[16384];
    int totalBytes = 0;
    std::string requestStr;

    // Read HTTP request header
    while (true) {
        int bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        requestStr.append(buffer, bytes);
        totalBytes += bytes;

        // Check if header complete
        size_t headerEnd = requestStr.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            // Check Content-Length to see if entire body is read
            size_t clPos = requestStr.find("Content-Length: ");
            if (clPos != std::string::npos) {
                size_t clEnd = requestStr.find("\r\n", clPos);
                int contentLen = std::stoi(requestStr.substr(clPos + 16, clEnd - clPos - 16));
                size_t bodyLen = requestStr.length() - (headerEnd + 4);
                if (bodyLen >= (size_t)contentLen) break;
            } else {
                break;
            }
        }
    }

    if (requestStr.empty()) {
        closesocket(clientSocket);
        return;
    }

    // Parse HTTP method and path
    std::istringstream reqStream(requestStr);
    std::string method, path, httpVersion;
    reqStream >> method >> path >> httpVersion;

    std::cout << "[INFO] " << method << " " << path << std::endl;

    // Route: GET /health
    if (method == "GET" && path == "/health") {
        std::string body = "{\"status\":\"UP\",\"service\":\"backend-cpp-render\",\"version\":\"1.0.0\"}\n";
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: application/json\r\n"
                 << "Content-Length: " << body.length() << "\r\n"
                 << "Access-Control-Allow-Origin: *\r\n"
                 << "Connection: close\r\n\r\n"
                 << body;
        std::string respStr = response.str();
        send(clientSocket, respStr.c_str(), (int)respStr.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // Route: OPTIONS (CORS preflight)
    if (method == "OPTIONS") {
        std::ostringstream response;
        response << "HTTP/1.1 204 No Content\r\n"
                 << "Access-Control-Allow-Origin: *\r\n"
                 << "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                 << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                 << "Connection: close\r\n\r\n";
        std::string respStr = response.str();
        send(clientSocket, respStr.c_str(), (int)respStr.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // Route: POST /render
    if (method == "POST" && (path == "/render" || path == "/api/render")) {
        size_t bodyPos = requestStr.find("\r\n\r\n");
        std::string body = (bodyPos != std::string::npos) ? requestStr.substr(bodyPos + 4) : "";

        // Extract metadata from JSON body
        std::string format = extractJsonString(body, "format");
        if (format.empty()) format = "PDF";
        std::transform(format.begin(), format.end(), format.begin(), ::toupper);

        std::string title = extractJsonString(body, "title");
        if (title.empty()) title = "Report Output";

        auto params = extractParameters(body);
        auto columns = extractJsonStringArray(body, "columns");
        if (columns.empty()) {
            columns = {"Column1", "Column2", "Column3", "Column4"};
        }

        std::string outputBytes;
        std::string contentType;

        if (format == "XLSX" || format == "CSV" || format == "EXCEL") {
            outputBytes = renderExcel(title, params, columns);
            contentType = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
        } else {
            outputBytes = renderPdf(title, params, columns);
            contentType = "application/pdf";
        }

        std::cout << "[SUCCESS] Rendered " << format << " report (" << outputBytes.length() << " bytes) for: " << title << std::endl;

        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: " << contentType << "\r\n"
                 << "Content-Length: " << outputBytes.length() << "\r\n"
                 << "Access-Control-Allow-Origin: *\r\n"
                 << "Connection: close\r\n\r\n";
        
        std::string headerStr = response.str();
        send(clientSocket, headerStr.c_str(), (int)headerStr.length(), 0);
        send(clientSocket, outputBytes.data(), (int)outputBytes.length(), 0);

        closesocket(clientSocket);
        return;
    }

    // 404 Not Found
    std::string notFound = "{\"error\":\"Not Found\"}\n";
    std::ostringstream response;
    response << "HTTP/1.1 404 Not Found\r\n"
             << "Content-Type: application/json\r\n"
             << "Content-Length: " << notFound.length() << "\r\n"
             << "Connection: close\r\n\r\n"
             << notFound;
    std::string respStr = response.str();
    send(clientSocket, respStr.c_str(), (int)respStr.length(), 0);
    closesocket(clientSocket);
}

int main(int argc, char* argv[]) {
    int port = 8081;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    std::cout << "==================================================" << std::endl;
    std::cout << "  C++ Report Rendering Microservice v1.0.0" << std::endl;
    std::cout << "  Listening on: http://0.0.0.0:" << port << std::endl;
    std::cout << "  Endpoint:     POST /render" << std::endl;
    std::cout << "==================================================" << std::endl;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }
#endif

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed on port " << port << std::endl;
        closesocket(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        closesocket(serverSocket);
        return 1;
    }

    std::cout << "[READY] Rendering server accepting connections..." << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        if (clientSocket != INVALID_SOCKET) {
            handleClient(clientSocket);
        }
    }

    closesocket(serverSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
