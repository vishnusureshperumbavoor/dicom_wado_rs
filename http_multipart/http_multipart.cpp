#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <fstream>
#include <algorithm>
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

size_t findSubstringIndex(const vector<char>& charVector,const string& searchString) {
    std::string vectorAsString(charVector.begin(), charVector.end());

    auto it = search(vectorAsString.begin(), vectorAsString.end(), searchString.begin(), searchString.end());

    if (it != vectorAsString.end()) {
        // Calculate the index by subtracting the iterator from the beginning of the string
        return distance(vectorAsString.begin(), it);
    }
    else {
        return -1; // Not found
    }
}

int findSubstringIndexStartingFrom(const std::vector<char>& charVector, const std::string& searchString, size_t startIndex) {
    if (startIndex >= charVector.size()) {
        return -1;  // Start index is out of bounds
    }

    std::string vectorAsString(charVector.begin() + startIndex, charVector.end());

    auto it = std::search(vectorAsString.begin(), vectorAsString.end(), searchString.begin(), searchString.end());

    if (it != vectorAsString.end()) {
        // Calculate the index by adding the start index to the distance from the beginning of the string
        return startIndex + std::distance(vectorAsString.begin(), it);
    }
    else {
        return -1; // Not found
    }
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

    cout << "contenttypeheader = " << contentTypeHeader << endl;

    // Read and display the response content
    char responseBuffer[4096]{};  // Adjust buffer size as needed
    DWORD bytesRead = 0;
    string responseBody;

    while (InternetReadFile(hConnect, responseBuffer, sizeof(responseBuffer), &bytesRead) && bytesRead > 0) {
        responseBody.append(responseBuffer, bytesRead);
        //cout.write(responseBuffer, bytesRead);
    }
    string boundary = extractBoundary(contentTypeHeader);

    // Split the response into parts using the boundary
    boundary = "--" + boundary;

    // Store the data in a vector of characters
    vector<char> data(responseBody.begin(), responseBody.end());
    const char* subString = boundary.c_str();
    size_t start = findSubstringIndex(data, subString);
    int partNumber = 1;

    //--414f2452-0bd2-4c59-b5b3-f285a6c6a9ab
    if (start != string::npos) {
        start += boundary.length();
        while (true) {
            size_t end = findSubstringIndexStartingFrom(data,boundary,start);
            if (end != string::npos) {
                size_t length = end - start;
                vector<char> extractedSubsequence(responseBody.begin() + start, responseBody.begin() + start + length);
                //string extractedString(extractedSubsequence.begin(), extractedSubsequence.end());
                //string dicomData = responseBody.substr(start, end - start);
                string fileName = "dicom_" + to_string(partNumber) + ".dcm";
                //saveDICOMToFile(extractedString, fileName);
                ofstream dicomFile(fileName, ios::out | ios::binary);
                dicomFile.write(extractedSubsequence.data(), extractedSubsequence.size());
                dicomFile.close();
                if (!dicomFile) {
                    std::cerr << "Error occurred while writing the DICOM file." << std::endl;
                    return 1;
                }

                std::cout << fileName << " created successfully." << std::endl;

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

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return 0;
}
