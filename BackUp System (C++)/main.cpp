#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>

#include <future>
#include <thread>
#include <atomic>
#include <execution>

#include <Windows.h>
#include <WinBase.h>
#include <tchar.h>

#import <msxml6.dll> rename_namespace(_T("MSXML"))

void findChildsInDirectory(std::wstring& directory, std::wstring& dstFolder, std::atomic_uint& threadsPending, std::atomic_bool& showErrors)
{
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::wstring childs = directory + L"\\*";

	hFind = FindFirstFileW(childs.c_str(), &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
		return;

	do
	{
		if (wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0) {
			std::wstring dstFile = dstFolder + L'\\' + ffd.cFileName;
			std::wstring srcFile = directory + L"\\" + ffd.cFileName;

			WIN32_FILE_ATTRIBUTE_DATA dstFileInfo;
			LARGE_INTEGER dstFileSize, dstFileTime;
			dstFileSize.QuadPart = 0; dstFileTime.QuadPart = 0;
			if (GetFileAttributesExW(dstFile.c_str(), GetFileExInfoStandard, &dstFileInfo)) {
				dstFileSize.LowPart = dstFileInfo.nFileSizeLow; dstFileSize.HighPart = dstFileInfo.nFileSizeHigh;
				dstFileTime.LowPart = dstFileInfo.ftLastWriteTime.dwLowDateTime; dstFileTime.HighPart = dstFileInfo.ftLastWriteTime.dwHighDateTime;
			}

			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if(dstFileTime.QuadPart == 0)
					CreateDirectoryW(dstFile.c_str(), NULL);

				findChildsInDirectory(srcFile, dstFile, threadsPending, showErrors);
			}
			else {
				LARGE_INTEGER srcFileSize, srcFileTime;
				srcFileSize.LowPart = ffd.nFileSizeLow; srcFileSize.HighPart = ffd.nFileSizeHigh;
				srcFileTime.LowPart = ffd.ftLastWriteTime.dwLowDateTime; srcFileTime.HighPart = ffd.ftLastWriteTime.dwHighDateTime;

				if ((dstFileSize.QuadPart == 0 && dstFileTime.QuadPart == 0) || srcFileSize.QuadPart != dstFileSize.QuadPart || srcFileTime.QuadPart > dstFileTime.QuadPart) {
					if (srcFileSize.QuadPart > 150000000) {
						threadsPending++;
						std::async(std::launch::async, [&threadsPending, &showErrors, &srcFile, &dstFile]() {
							BOOL result = CopyFileExW(srcFile.c_str(), dstFile.c_str(), FALSE, NULL, NULL, COPY_FILE_NO_BUFFERING);
							if (showErrors && result == 0)
								std::wcerr << std::wstring(L"Error in file: " + srcFile + L" Error: " + std::to_wstring(GetLastError())) << std::endl;
							threadsPending--;
						});
					}
					else {
						threadsPending++;
						std::async(std::launch::async, [&threadsPending, &showErrors, &srcFile, &dstFile]() {
							BOOL result = CopyFileW(srcFile.c_str(), dstFile.c_str(), FALSE);
							if (showErrors && result == 0)
								std::wcerr << std::wstring(L"Error in file: " + srcFile + L" Error: " + std::to_wstring(GetLastError())) << std::endl;
							threadsPending--;
						});
					}
				}
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	DWORD dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES) {
		std::cerr << "Error: " << dwError << std::endl;
		return;
	}

	FindClose(hFind);
}

void findToDeleteChildsInDirectory(std::wstring& directory, std::wstring& srcFolder, std::atomic_uint& threadsPending, std::atomic_bool& showErrors)
{
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::wstring childs = directory + L"\\*";

	hFind = FindFirstFileW(childs.c_str(), &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
		return;

	do
	{
		if (wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0) {
			std::wstring srcFile = srcFolder + L'\\' + ffd.cFileName;
			std::wstring dstFile = directory + L"\\" + ffd.cFileName;

			WIN32_FILE_ATTRIBUTE_DATA srcFileInfo;
			LARGE_INTEGER srcFileSize, srcFileTime;
			srcFileSize.QuadPart = 0; srcFileTime.QuadPart = 0;
			if (GetFileAttributesExW(srcFile.c_str(), GetFileExInfoStandard, &srcFileInfo)) {
				srcFileSize.LowPart = srcFileInfo.nFileSizeLow; srcFileSize.HighPart = srcFileInfo.nFileSizeHigh;
				srcFileTime.LowPart = srcFileInfo.ftLastWriteTime.dwLowDateTime; srcFileTime.HighPart = srcFileInfo.ftLastWriteTime.dwHighDateTime;
			}

			if (srcFileSize.QuadPart == 0 && srcFileTime.QuadPart == 0) {
				if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					findToDeleteChildsInDirectory(dstFile, srcFile, threadsPending, showErrors);

					threadsPending++;
					std::async(std::launch::async, [&threadsPending, &showErrors, &dstFile]() {
						BOOL result = RemoveDirectoryW(dstFile.c_str());
						if (showErrors && result == 0)
							std::wcerr << std::wstring(L"Error removing directory: " + dstFile + L" Error: " + std::to_wstring(GetLastError())) << std::endl;
						threadsPending--;
					});
				}
				else {
					threadsPending++;
					std::async(std::launch::async, [&threadsPending, &showErrors, &dstFile]() {
						BOOL result = DeleteFileW(dstFile.c_str());
						if (showErrors && result == 0)
							std::wcerr << std::wstring(L"Error removing file: " + dstFile + L" Error: " + std::to_wstring(GetLastError())) << std::endl;
						threadsPending--;
					});
				}
			}
			else {
				findToDeleteChildsInDirectory(dstFile, srcFile, threadsPending, showErrors);
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	DWORD dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES) {
		std::cerr << "Error: " << dwError << std::endl;
		return;
	}

	FindClose(hFind);
}

int main(int argc, char* argv[])
{
	ShowWindow(GetConsoleWindow(), SW_FORCEMINIMIZE);

	std::atomic_bool showErrors = true;

	if (argc < 2 || argc > 3) {
		std::cerr << "Wrong number of arguments, usage: [backupConfigFile.xml] [-ne]" << std::endl;
		return 1;
	}

	switch (argc)
	{
	case 2:
		if (std::string(argv[1]) == "-ne") {
			std::cerr << "Wrong number of arguments, usage: [backupConfigFile.xml] [-ne]" << std::endl;
			return 1;
		}
		break;
	case 3:
		if (std::string(argv[2]) == "-ne") {
			showErrors = false;
		}
		break;
	default:
		break;
	}

	std::vector<std::string> srcFolders;
	std::vector<std::string> dstFolders;

	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		std::cerr << "Failed to CoInitialize: " << hr << std::endl;
		return 1;
	}
	try {
		MSXML::IXMLDOMDocument2Ptr xmlDoc;
		hr = xmlDoc.CreateInstance(__uuidof(MSXML::DOMDocument60), NULL, CLSCTX_INPROC_SERVER);

		variant_t xmlFileName;
		xmlFileName.SetString(std::string(argv[1]).c_str());
		if (xmlDoc->load(xmlFileName) != VARIANT_TRUE) {
			std::cerr << "Unable to load file: " << std::string(argv[1]) << std::endl;
			return 1;
		}
		else {
			MSXML::IXMLDOMNodeListPtr xmlSrcFolders = xmlDoc->selectNodes("//sourceFolders/folder");
			if (xmlSrcFolders->Getlength() == 0) {
				std::cerr << "Source folders cant be empty" << std::endl;
				return 1;
			}

			for (int i = 0; i < xmlSrcFolders->Getlength(); i++) {
				srcFolders.push_back(std::string(xmlSrcFolders->Getitem(i)->text));
			}

			MSXML::IXMLDOMNodeListPtr xmlDstFolders = xmlDoc->selectNodes("//destinationFolders/folder");
			if (xmlDstFolders->Getlength() == 0) {
				std::cerr << "Destination folders cant be empty" << std::endl;
				return 1;
			}

			for (int i = 0; i < xmlDstFolders->Getlength(); i++) {
				dstFolders.push_back(std::string(xmlDstFolders->Getitem(i)->text));
			}
		}
	}
	catch (...) {
		std::cerr << "Failed to read config xml" << std::endl;
		return 1;
	}
	CoUninitialize();	

	/*srcFolders = {
		"C:\\Prueba1",
		"C:\\Prueba2",
		"C:\\Prueba3"
	};

	dstFolders = {
		"D:\\Backup Raid"
	};*/

	std::cout << "Starting to BackUp..." << std::endl;

	std::atomic_uint dstFoldersPending(static_cast<unsigned int>(dstFolders.size()));
	std::atomic_uint srcFoldersPending(static_cast<unsigned int>(srcFolders.size() * dstFolders.size()));
	std::atomic_uint threadsPending(0);

	std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();

	try
	{
		std::for_each(std::execution::par_unseq, dstFolders.begin(), dstFolders.end(), [&srcFolders, &srcFoldersPending, &dstFoldersPending, &threadsPending, &showErrors](auto& dstFolder) {
		
			int dstCharsNum = MultiByteToWideChar(CP_UTF8, 0, dstFolder.c_str(), -1, NULL, 0);
			wchar_t* dstWstr = new wchar_t[dstCharsNum];
			MultiByteToWideChar(CP_UTF8, 0, dstFolder.c_str(), -1, dstWstr, dstCharsNum);
			std::wstring dstFolderW(dstWstr);
			delete[] dstWstr;

			std::for_each(std::execution::par_unseq, srcFolders.begin(), srcFolders.end(), [&dstFolderW, &srcFoldersPending, &dstFoldersPending, &threadsPending, &showErrors](auto& srcFolder) {

				int srcCharsNum = MultiByteToWideChar(CP_UTF8, 0, srcFolder.c_str(), -1, NULL, 0);
				wchar_t* srcWstr = new wchar_t[srcCharsNum];
				MultiByteToWideChar(CP_UTF8, 0, srcFolder.c_str(), -1, srcWstr, srcCharsNum);
				std::wstring srcFolderW(srcWstr);
				delete[] srcWstr;

				size_t parentFolderSize = srcFolderW.substr(0, srcFolderW.find_last_of(L"\\") + 1).length();
				std::wstring dstRootFolderW = dstFolderW + L'\\' + (srcFolderW.substr(parentFolderSize));

				WIN32_FILE_ATTRIBUTE_DATA dstFileInfo;
				LARGE_INTEGER dstFileTime;
				dstFileTime.QuadPart = 0;
				if (GetFileAttributesExW(dstRootFolderW.c_str(), GetFileExInfoStandard, &dstFileInfo)) {
					dstFileTime.LowPart = dstFileInfo.ftLastWriteTime.dwLowDateTime; dstFileTime.HighPart = dstFileInfo.ftLastWriteTime.dwHighDateTime;
				}

				if (dstFileTime.QuadPart == 0) {
					CreateDirectoryW(dstRootFolderW.c_str(), NULL);
				}
				findChildsInDirectory(srcFolderW, dstRootFolderW, threadsPending, showErrors);
				srcFoldersPending--;
			});
			dstFoldersPending--;
		});

		while (true)
		{
			if (threadsPending == 0 && srcFoldersPending == 0 && dstFoldersPending == 0)
				break;

			std::this_thread::sleep_for(std::chrono::milliseconds(150));
		}

		dstFoldersPending = static_cast<unsigned int>(dstFolders.size());
		srcFoldersPending = static_cast<unsigned int>(srcFolders.size() * dstFolders.size());
		threadsPending = 0;

		std::for_each(std::execution::par_unseq, dstFolders.begin(), dstFolders.end(), [&srcFolders, &srcFoldersPending, &dstFoldersPending, &threadsPending, &showErrors](auto& dstFolder) {

			int dstCharsNum = MultiByteToWideChar(CP_UTF8, 0, dstFolder.c_str(), -1, NULL, 0);
			wchar_t* dstWstr = new wchar_t[dstCharsNum];
			MultiByteToWideChar(CP_UTF8, 0, dstFolder.c_str(), -1, dstWstr, dstCharsNum);
			std::wstring dstFolderW(dstWstr);
			delete[] dstWstr;

			std::for_each(std::execution::par_unseq, srcFolders.begin(), srcFolders.end(), [&dstFolderW, &srcFoldersPending, &dstFoldersPending, &threadsPending, &showErrors](auto& srcFolder) {

				int srcCharsNum = MultiByteToWideChar(CP_UTF8, 0, srcFolder.c_str(), -1, NULL, 0);
				wchar_t* srcWstr = new wchar_t[srcCharsNum];
				MultiByteToWideChar(CP_UTF8, 0, srcFolder.c_str(), -1, srcWstr, srcCharsNum);
				std::wstring srcFolderW(srcWstr);
				delete[] srcWstr;

				size_t parentFolderSize = srcFolderW.substr(0, srcFolderW.find_last_of(L"\\") + 1).length();
				std::wstring dstRootFolderW = dstFolderW + L'\\' + (srcFolderW.substr(parentFolderSize));

				WIN32_FILE_ATTRIBUTE_DATA dstFileInfo;
				LARGE_INTEGER dstFileTime;
				dstFileTime.QuadPart = 0;
				if (GetFileAttributesExW(dstRootFolderW.c_str(), GetFileExInfoStandard, &dstFileInfo)) {
					dstFileTime.LowPart = dstFileInfo.ftLastWriteTime.dwLowDateTime; dstFileTime.HighPart = dstFileInfo.ftLastWriteTime.dwHighDateTime;
				}

				if (dstFileTime.QuadPart == 0) {
					CreateDirectoryW(dstRootFolderW.c_str(), NULL);
				}
				findToDeleteChildsInDirectory(dstRootFolderW, srcFolderW, threadsPending, showErrors);
				srcFoldersPending--;
			});
			dstFoldersPending--;
		});

		while (true)
		{
			if (threadsPending == 0 && srcFoldersPending == 0 && dstFoldersPending == 0)
				break;

			std::this_thread::sleep_for(std::chrono::milliseconds(150));
		}
	}
	catch (std::exception ex) {
		std::cerr << "Failed to copy" << std::endl;
		return 1;
	}

	end = std::chrono::system_clock::now();

	long long elapsedTimeH = std::chrono::duration_cast<std::chrono::hours>(end - start).count();
	long long elapsedTimeM = std::chrono::duration_cast<std::chrono::minutes>(end - start).count();
	long long elapsedTimeS = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
	long long elapsedTimeMS = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	std::cout << "Time used: " << elapsedTimeH << " h " << elapsedTimeM << " m " << elapsedTimeS << " s " << elapsedTimeMS % 1000 << " ms" << std::endl;

	system("PAUSE");

	// Copy errors: 3-5-32-1920
	// Remove errors: 145
}