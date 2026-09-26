#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <queue>
#include <vector>
#include <chrono>
#include <cmath>
#include <iomanip>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;


struct VehicleRecord {
    std::string plate;
    int slotNumber;
    TimePoint entryTime;
};

class ParkingSystem {
private:
    int totalSlots;
    int availableSlots;


     std::priority_queue<int, std::vector<int>, std::greater<int>> freeSlots;

     std::unordered_map<std::string, VehicleRecord> activeVehicles;


      static double calculateFee(double minutesParked) {
        if (minutesParked <= 30.0)        return 0.0;
        else if (minutesParked <= 120.0)  return 50.0;
        else if (minutesParked <= 240.0)  return 100.0;
        else if (minutesParked <= 360.0)  return 300.0;
        else                               return 500.0;
    }

public:
    ParkingSystem(int slots)
        : totalSlots(slots), availableSlots(slots) {
        for (int i = 1; i <= slots; ++i) freeSlots.push(i);
    }

    int total() const { return totalSlots; }
    int slotsAvailable() const { return availableSlots; }
    bool isAvailable() const { return availableSlots > 0; }

    struct Result {
        bool success;
        int slot = -1;
        double amount = -1.0;
        double minutes = -1.0;
        std::string message;
    };

    Result vehicleEntry(const std::string& plate) {
        if (!isAvailable())
            return {false, -1, -1, -1, "Entry denied: lot is full."};
        if (activeVehicles.count(plate))
            return {false, -1, -1, -1, "Entry denied: " + plate + " is already parked."};

        int slot = freeSlots.top();
        freeSlots.pop();
        
         activeVehicles[plate] = VehicleRecord{plate, slot, Clock::now()};
        availableSlots--;

        std::ostringstream msg;
        msg << "Vehicle " << plate << " parked in slot " << slot << ".";
        return {true, slot, 0.0, 0.0, msg.str()};
    }

    Result vehicleExit(const std::string& plate) {
        auto it = activeVehicles.find(plate);
        if (it == activeVehicles.end())
            return {false, -1, -1, -1, "Exit denied: no active record for " + plate + "."};

        VehicleRecord record = it->second;
        TimePoint exitTime = Clock::now();

        double minutesParked = std::chrono::duration<double>(exitTime - record.entryTime).count() / 60.0;
        double amount = calculateFee(minutesParked);

         activeVehicles.erase(it);
        freeSlots.push(record.slotNumber);
        availableSlots++;

        std::ostringstream msg;
        msg << std::fixed << std::setprecision(2);
        msg << "Vehicle " << plate << " exited slot " << record.slotNumber
            << ". Time parked: " << minutesParked << " min. Amount due: Ksh " << amount << ".";
        return {true, record.slotNumber, amount, minutesParked, msg.str()};
    }
};

static std::string urlDecode(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '+') {
            out += ' ';
        } else if (in[i] == '%' && i + 2 < in.size()) {
            int value = std::stoi(in.substr(i + 1, 2), nullptr, 16);
            out += static_cast<char>(value);
            i += 2;
        } else {
            out += in[i];
        }
    }
    return out;}

    static std::string getFormField(const std::string& body, const std::string& key) {
    size_t pos = body.find(key + "=");
    if (pos == std::string::npos) return "";
    pos += key.size() + 1;
    size_t end = body.find('&', pos);
    std::string raw = body.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
    return urlDecode(raw);
}

static std::string htmlEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '&': out += "&amp;"; break;
            case '"': out += "&quot;"; break;
            default: out += c;

               }
    }
    return out;
}

static std::string renderPage(ParkingSystem& lot, const std::string& notice) {
    std::ostringstream html;
    html << R"HTML(<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Parking System</title>
  <style>
    body { font-family: Arial, sans-serif; background:#f4f5f7; margin:0; padding:40px; }
    .card { max-width:480px; margin:0 auto; background:#fff; border-radius:10px;
            box-shadow:0 2px 8px rgba(0,0,0,0.1); padding:24px 28px; }
    h1 { font-size:20px; margin-top:0; }
    .status { font-size:16px; margin-bottom:20px; }
    .status b { color:#1a73e8; }
    form { margin-bottom:16px; display:flex; gap:8px; }
    input[type=text] { flex:1; padding:8px; border:1px solid #ccc; border-radius:6px; }
    button { padding:8px 14px; border:none; border-radius:6px; background:#1a73e8; color:#fff; cursor:pointer; }
    button:hover { background:#1558b0; }
    .exit-btn { background:#d93025; }
    .exit-btn:hover { background:#a52218; }
    .notice { background:#eef7ee; border:1px solid #bfe3bf; padding:10px 14px;
              border-radius:6px; margin-bottom:16px; font-size:14px; }
    table { width:100%; border-collapse:collapse; margin-top:10px; font-size:14px; }
    th, td { text-align:left; padding:4px 0; }
  </style>
</head>
<body>
  <div class="card">
    <h1>Parking System</h1>
)HTML";

    if (!notice.empty()) {
        html << "    <div class=\"notice\">" << htmlEscape(notice) << "</div>\n";
    }

    html << "    <div class=\"status\">Available slots: <b>" << lot.slotsAvailable()
         << "</b> / " << lot.total() << "</div>\n";

    html << R"HTML(
    <form method="POST" action="/entry">
      <input type="text" name="plate" placeholder="Plate number" required>
      <button type="submit">Vehicle entry</button>
    </form>

    <form method="POST" action="/exit">
      <input type="text" name="plate" placeholder="Plate number" required>
      <button type="submit" class="exit-btn">Vehicle exit</button>
    </form>

    <table>
      <tr><th>0 - 30 min</th><td>Free</td></tr>
      <tr><th>30 min - 2 hrs</th><td>Ksh 50</td></tr>
      <tr><th>2 - 4 hrs</th><td>Ksh 100</td></tr>
      <tr><th>4 - 6 hrs</th><td>Ksh 300</td></tr>
      <tr><th>Above 6 hrs</th><td>Ksh 500</td></tr>
    </table>
  </div>
</body>
</html>
)HTML";

    return html.str();
}

static std::string httpResponse(const std::string& body, const std::string& status = "200 OK") {
    std::ostringstream res;
    res << "HTTP/1.1 " << status << "\r\n"
        << "Content-Type: text/html; charset=utf-8\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n\r\n"
        << body;
    return res.str();
}

int main() {
    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to start Windows networking\n";
        return 1;
    }

    ParkingSystem lot(15);

    SOCKET serverFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverFd == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        WSACleanup();
        return 1;
    }

    BOOL option = TRUE;
    setsockopt(
        serverFd,
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<const char*>(&option),
        sizeof(option)
    );

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))
        == SOCKET_ERROR) {
        std::cerr << "Could not use port 8080\n";
        closesocket(serverFd);
        WSACleanup();
        return 1;
    }

    if (listen(serverFd, 10) == SOCKET_ERROR) {
        std::cerr << "Failed to start listening\n";
        closesocket(serverFd);
        WSACleanup();
        return 1;
    }

    std::cout << "Parking system running at http://localhost:8080/\n";

    while (true) {
        SOCKET client = accept(serverFd, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            continue;
        }

        std::string request;
        char buffer[4096];
        int received = 0;
        size_t contentLength = 0;
        bool headersDone = false;

        while ((received = recv(client, buffer, sizeof(buffer), 0)) > 0) {
            request.append(buffer, received);

            if (!headersDone) {
                size_t headerEnd = request.find("\r\n\r\n");
                if (headerEnd != std::string::npos) {
                    headersDone = true;
                    size_t lengthPosition = request.find("Content-Length:");

                    if (lengthPosition != std::string::npos &&
                        lengthPosition < headerEnd) {
                        contentLength =
                            std::stoul(request.substr(lengthPosition + 16));
                    }
                }
            }

            if (headersDone) {
                size_t bodyStart = request.find("\r\n\r\n") + 4;
                if (request.size() - bodyStart >= contentLength) {
                    break;
                }
            }
        }

        std::istringstream requestStream(request);
        std::string method, path, httpVersion;
        requestStream >> method >> path >> httpVersion;

        std::string body;
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            body = request.substr(headerEnd + 4);
        }

        std::string responseHtml;

        if (method == "GET" && path == "/") {
            responseHtml = renderPage(lot, "");
        } else if (method == "POST" && path == "/entry") {
            std::string plate = getFormField(body, "plate");
            auto result = lot.vehicleEntry(plate);
            responseHtml = renderPage(lot, result.message);
        } else if (method == "POST" && path == "/exit") {
            std::string plate = getFormField(body, "plate");
            auto result = lot.vehicleExit(plate);
            responseHtml = renderPage(lot, result.message);
        } else {
            responseHtml = renderPage(lot, "Page not found.");
        }

        std::string response = httpResponse(responseHtml);
        send(client, response.c_str(), static_cast<int>(response.size()), 0);
        closesocket(client);
    }

    closesocket(serverFd);
    WSACleanup();
    return 0;
}





    



