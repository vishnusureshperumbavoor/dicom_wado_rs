#include <windows.h>
#include <wininet.h>
#include <iostream>
using namespace std;

int main() {
    HINTERNET hInternet, hConnect;

    INTERNET_PROXY_INFO proxyInfo;
    proxyInfo.dwAccessType = INTERNET_OPEN_TYPE_PROXY;
    proxyInfo.lpszProxy = L"127.0.0.1:8888";
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
    if (HttpQueryInfo(hConnect, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL) == TRUE) {
        if (statusCode == 200) {
            cout << "HTTP request was successful (Status Code 200 OK)." << endl;
        }
        else {
            cout << "HTTP request was not successful. Status Code: " << statusCode << endl;
        }
    }

    // Read and display the response content
    char responseBuffer[4096];  // Adjust buffer size as needed
    DWORD bytesRead = 0;
    while (InternetReadFile(hConnect, responseBuffer, sizeof(responseBuffer), &bytesRead) && bytesRead > 0) {
        cout.write(responseBuffer, bytesRead);
    }

    // Continue with your HTTP request handling...

    // Don't forget to close handles when you're done
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return 0;
}
