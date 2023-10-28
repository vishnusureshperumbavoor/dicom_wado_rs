#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
using namespace std;

size_t isSubstring(const std::string& s2, const std::string& s1)
{
    int M = s1.length();
    int N = s2.length();

    for (int i = 0; i <= N - M; i++) {
        int j;
        for (j = 0; j < M; j++)
            if (s2[i + j] != s1[j])
                break;

        if (j == M)
            return i;
    }

    return -1;
}

// Function to extract the boundary parameter from the Content-Type header
string extractBoundary(string contentType) {
    size_t startPos = contentType.find("\"");
    cout << startPos << endl;

    if (startPos != string::npos) {
        size_t endPos = contentType.find("\"", startPos + 1);
        if (endPos != string::npos) {
            string substring = "--";
            substring += contentType.substr(startPos + 1, endPos - startPos - 1);
            return substring;
        }
    }
    return "";
}

int main() {
    HINTERNET hInternet, hConnect;

    INTERNET_PROXY_INFO proxyInfo;
    proxyInfo.dwAccessType = INTERNET_OPEN_TYPE_PROXY;
    proxyInfo.lpszProxy = L"127.0.0.1:8888";  // Assuming Fiddler is running on the default port
    proxyInfo.lpszProxyBypass = L"<local>";

    // Initialize WinINet with proxy settings
    hInternet = InternetOpen(L"MyApp", INTERNET_OPEN_TYPE_DIRECT, proxyInfo.lpszProxy, proxyInfo.lpszProxyBypass, 0);
    if (hInternet == NULL) {
        // Handle error
        cout << "InternetOpen failed: " << GetLastError() << endl;
        return 1;
    }

    // Create an HTTP connection
    hConnect = InternetOpenUrl(hInternet, L"https://dicomserver.co.uk:8989/wado/studies/1.2.840.113619.2.3.281.8005.2001.11.14.45",
        NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (hConnect == NULL) {
        // Handle error
        cout << "InternetOpenUrl failed: " << GetLastError() << endl;
        InternetCloseHandle(hInternet);
        return 1;
    }

    // Set the "Accept" header
    LPCWSTR acceptHeaders = L"Accept: multipart/related; type=\"application/dicom\"\r\n";
    if (!HttpAddRequestHeaders(hConnect, acceptHeaders, -1, HTTP_ADDREQ_FLAG_ADD)) {
        // Handle error
        cout << "HttpAddRequestHeaders failed: " << GetLastError() << endl;
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return 1;
    }

    // Send the request
    if (!HttpSendRequest(hConnect, NULL, 0, NULL, 0)) {
        // Handle error
        cout << "HttpSendRequest failed: " << GetLastError() << endl;
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return 1;
    }

    // Check the response status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    string contentTypeHeader;
    if (HttpQueryInfo(hConnect, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL) == TRUE) {
        if (statusCode == 200) {
            cout << "HTTP request was successful (Status Code 200 OK)." << endl;
            char contentTypeBuffer[1024]{};
            DWORD bufferSize = sizeof(contentTypeBuffer);

            if (HttpQueryInfo(hConnect, HTTP_QUERY_CONTENT_TYPE, contentTypeBuffer, &bufferSize, NULL)) {
                contentTypeHeader = string(contentTypeBuffer, bufferSize);
            }
            else {
                cout << "Content - Type header is not found or an error occurred" << endl;
            }

        }
        else {
            cout << "HTTP request was not successful. Status Code: " << statusCode << endl;
        }
    }

    // Read and display the response content
    char responseBuffer[4096]{};  // Adjust buffer size as needed
    DWORD bytesRead = 0;
    string responseBody;

    while (InternetReadFile(hConnect, responseBuffer, sizeof(responseBuffer), &bytesRead) && bytesRead > 0) {
        responseBody.append(responseBuffer, bytesRead);
        //cout.write(responseBuffer, bytesRead);
    }

    string boundary = extractBoundary(contentTypeHeader);
    cout << boundary << endl;

    // Split the response into parts using the boundary
    size_t start = responseBody.find(boundary);
    //size_t start = isSubstring(responseBody, "--" + boundary);
    size_t end = responseBody.find("--" + boundary + "--");
    cout << "start = " << start << endl << " end = " << end << endl;

    int partNumber = 1;

    while (start != string::npos && end != string::npos) {
        string part = responseBody.substr(start, end - start);
        cout << "Part : " << partNumber << endl << part << endl;

        // Find the next part
        start = responseBody.find("--" + boundary, end);
        end = responseBody.find("--" + boundary + "--", start);
        partNumber++;
    }

    // Continue with your HTTP request handling...

    // Don't forget to close handles when you're done
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return 0;
}
