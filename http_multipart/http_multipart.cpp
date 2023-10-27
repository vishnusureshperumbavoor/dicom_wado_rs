#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <fstream>
using namespace std;

// Function to extract the boundary parameter from the Content-Type header
string extractBoundary(string contentType) {
    size_t startPos = contentType.find("\"");
    if (startPos != std::string::npos) {
        size_t endPos = contentType.find("\"", startPos + 1);
        if (endPos != std::string::npos) {
            std::string substring = contentType.substr(startPos + 1, endPos - startPos - 1);
            return substring;
        }
    }
    return "";
}

int findSubstringIndex(const char* mainString, const char* substring) {
    cout << "mainstring" << endl;
    cout << "---------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << mainString << endl;
    cout << "------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << "substring = " << substring << endl;
    const char* mainPtr = mainString;
    int index = 0;
    while (*mainPtr != '\0') {
        const char* subPtr = substring;
        const char* tempMainPtr = mainPtr;
        while (*subPtr != '\0' && *tempMainPtr == *subPtr) {
            tempMainPtr++;
            subPtr++;
        }
        if (*subPtr == '\0') {
            return index;
        }
        mainPtr++;
        index++;
    }
    return -1;
}

void saveDICOMToFile(const string& dicomData, const string& filename) {
    fstream outFile(filename, ios::out | ios::binary);
    if (outFile.is_open()) {
        outFile.write(dicomData.c_str(), dicomData.size());
        outFile.close();
        cout << "Saved DICOM data to " << filename << endl;
    }
    else {
        cerr << "Error : unable to open file or writing" << endl;
    }
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
    hConnect = InternetOpenUrl(hInternet, L"https://dicomserver.co.uk:8989/wado/studies/2.25.226245767546263669921319690953240842662",
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
    DWORD dwBufferLength = 0;
    char* pszContentType = nullptr;

    if (!HttpQueryInfo(hConnect, HTTP_QUERY_CONTENT_TYPE, pszContentType, &dwBufferLength, NULL)) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            cout << "HTTP request was successful (Status Code 200 OK)." << endl;
            pszContentType = new char[dwBufferLength];

            //char contentTypeBuffer[1024]{};
            //DWORD bufferSize = sizeof(contentTypeBuffer);

            if (HttpQueryInfo(hConnect, HTTP_QUERY_CONTENT_TYPE, pszContentType, &dwBufferLength, NULL)) {
                //contentTypeHeader = string(contentTypeBuffer, bufferSize);
                for (DWORD i = 0; i < dwBufferLength; i++) {
                    if (pszContentType[i] != '\0') {
                        contentTypeHeader += pszContentType[i];
                    }
                }
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


    cout << "----------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << "--" << boundary << endl;
    cout << responseBody << endl;
    cout << "--" << boundary << "--" << endl;
    cout << "----------------------------------------------------------------------------------------------------------------------------------------------------" << endl;


    // Split the response into parts using the boundary
    boundary = "--" + boundary;
    const char* mainString = responseBody.c_str();
    const char* subString = boundary.c_str();
    size_t start = findSubstringIndex(mainString, subString);
    cout << "boundary length = " << boundary.length() << endl;
    if (start != -1) {
        cout << "start found at index " << start << endl;
    }
    else {
        cout << "start not found" << endl;
    }

    size_t end = responseBody.find("--" + boundary, start);

    cout << "end = " << end << endl;

    int partNumber = 1;

    if (start != string::npos) {
        start += boundary.length();
        while (true) {
            size_t end = responseBody.find(boundary, start);
            if (end != string::npos) {
                string dicomData = responseBody.substr(start, end - start);
                string fileName = "dicom_" + to_string(partNumber) + ".dcm";
                saveDICOMToFile(dicomData, fileName);
                start = end + boundary.length();
                partNumber++;
            }
            else {
                break;
            }
        }
    }
    else {
        cout << "no dicom parts found in the response" << endl;
    }

    // Continue with your HTTP request handling...

    // Don't forget to close handles when you're done
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return 0;
}
