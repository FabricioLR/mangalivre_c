//gcc window.c -o window -lgdi32 -lcomdlg32 -LC:\curl\lib -lcurl cJSON.c -ljpeg
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <combaseapi.h>
#include <unistd.h>
#include <strings.h>
#include <curl/curl.h>
#include <libjpeg/jpeglib.h>
#include <unistd.h>
#include "cjson.h"

#define BUFFER_SIZE 1024

void ShowError(char *message);
int WinMain2(WNDCLASSEX wincl_2, HINSTANCE hInstance, int nCmdShow);
void add_buttons(HWND hwnd, HINSTANCE hInstance);
void add_elements(HWND hwnd, HINSTANCE hInstance);
const char* OpenFileDialog(HWND hwnd);
HBITMAP ShowImage(HWND hwnd);
int convert_to_bmp(char *sourcefilename, char *destfilename);
static size_t cb(void *data, size_t size, size_t nmemb, void *clientp);
size_t write_data_to_file(void *ptr, size_t size, size_t nmemb, FILE *stream);
int DownloadImage(const char *url);

TCHAR types[4][10] = {"manga", "manhua", "webtoon", "novel"};
TCHAR periods[4][10] = {"day", "week", "mounth", "year"};
char type[12] = "manga";
char period[12] = "week";

int windowIndex = 0;

const char g_szClassName[] = "myWindowClass";
const char g_szClassName2[] = "myWindowClass2";

typedef struct {
    long filesize;
    char reserved[2];
    long headersize;
    long infoSize;
    long width;
    long depth;
    short biPlanes;
    short bits;
    long biCompression;
    long biSizeImage;
    long biXPelsPerMeter;
    long biYPelsPerMeter;
    long biClrUsed;
    long biClrImportant;
} BMPHEAD;

struct memory {
    char *response;
    size_t size;
};

int serie_index;
struct memory chunk = {0};

LRESULT CALLBACK WndProc2(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch (msg){
        case WM_COMMAND:
            break;
        case WM_CREATE:
            break;
        case WM_DESTROY:
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    static HBITMAP hBitmap;
    HDC hdc;
    PAINTSTRUCT ps;
    BITMAP bitmap;
    HDC hdcMem;
    HGDIOBJ oldBitmap;
    CURL *curl;
    CURLcode res;
    cJSON *json;
    cJSON *most_read;
    cJSON *subitem_most_read;
    cJSON *series_image;
    int i, downloadImageStatus;

    switch(msg){
        case WM_CREATE:
            add_elements(hwnd, NULL);
            break;
        case WM_COMMAND:
            if(HIWORD(wParam) == CBN_SELCHANGE){
                if (LOWORD(wParam) == 11){
                    int ItemIndex = SendMessage((HWND) lParam, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                    strcpy(type, types[ItemIndex]);
                }
                if (LOWORD(wParam) == 12){
                    int ItemIndex = SendMessage((HWND) lParam, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                    strcpy(period, periods[ItemIndex]);
                }
            }

            if ((HIWORD(wParam) == BN_CLICKED) && (lParam != 0)){
                if (LOWORD(wParam) == 0){
                    curl = curl_easy_init();
                    if(curl) {
                        char url[150];
                        sprintf(url, "https://mangalivre.net/home/most_read_period?period=%s&type=%s", period, type);
                        curl_easy_setopt(curl, CURLOPT_URL, url);
                        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 1);  
                        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);  
                        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);        
                        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
                        curl_easy_setopt(curl, CURLOPT_CAPATH, "cacert.pem");
                    
                        res = curl_easy_perform(curl);

                        if(res != CURLE_OK){
                            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                        }

                        json = cJSON_Parse(chunk.response);
                        most_read = cJSON_GetObjectItemCaseSensitive(json, "most_read");

                        serie_index = 0;

                        downloadImageStatus = DownloadImage(cJSON_GetObjectItem(cJSON_GetArrayItem(most_read, serie_index), "series_image")->valuestring);
                        
                        if (downloadImageStatus < 1){ 
                            ShowError("error downloading file");
                            exit(1);
                        }

                        if (convert_to_bmp("serie.jpg", "serie.bmp") < 1){
                            ShowError("convert to bmp error");
                            exit(1);
                        }

                        //for (i = 0 ; i < cJSON_GetArraySize(most_read) ; i++){
                        //    subitem_most_read = cJSON_GetArrayItem(most_read, 0);
                        //    series_image = cJSON_GetObjectItem(subitem_most_read, "series_image");
                        //    //printf(series_image->valuestring);
                        //}

                        hBitmap = ShowImage(hwnd);

                        if (hBitmap == NULL){
                            printf("bitmap null");
                        }

                        InvalidateRect(hwnd, NULL, TRUE);

                        add_buttons(hwnd, NULL);

                        cJSON_Delete(json);
                    
                        curl_easy_cleanup(curl);
                    }
                    break;
                }
                if (LOWORD(wParam) == 1){
                    json = cJSON_Parse(chunk.response);
                    most_read = cJSON_GetObjectItemCaseSensitive(json, "most_read");

                    if ((serie_index - 1) >= 0){
                        serie_index = serie_index - 1;

                        downloadImageStatus = DownloadImage(cJSON_GetObjectItem(cJSON_GetArrayItem(most_read, serie_index), "series_image")->valuestring);
                        
                        if (downloadImageStatus < 1){ 
                            ShowError("error downloading file");
                            exit(1);
                        }

                        if (convert_to_bmp("serie.jpg", "serie.bmp") < 1){
                            ShowError("convert to bmp error");
                            exit(1);
                        }

                        hBitmap = ShowImage(hwnd);

                        if (hBitmap == NULL){
                            printf("bitmap null");
                        }

                        InvalidateRect(hwnd, NULL, TRUE);
                        
                        break;
                    }
                }
                if (LOWORD(wParam) == 2){
                    json = cJSON_Parse(chunk.response);
                    most_read = cJSON_GetObjectItemCaseSensitive(json, "most_read");
    
                    if ((serie_index + 1) < cJSON_GetArraySize(most_read)){
                        serie_index = serie_index + 1;

                        downloadImageStatus = DownloadImage(cJSON_GetObjectItem(cJSON_GetArrayItem(most_read, serie_index), "series_image")->valuestring);
                        
                        if (downloadImageStatus < 1){ 
                            ShowError("error downloading file");
                            exit(1);
                        }

                        if (convert_to_bmp("serie.jpg", "serie.bmp") < 1){
                            ShowError("convert to bmp error");
                            exit(1);
                        }

                        hBitmap = ShowImage(hwnd);

                        if (hBitmap == NULL){
                            printf("bitmap null");
                        }

                        InvalidateRect(hwnd, NULL, TRUE);

                        break;
                    }
                }
                if (LOWORD(wParam) == 3){
                    WNDCLASSEX w2;
                    HINSTANCE hInstance = (HINSTANCE)GetWindowLongA(hwnd, -6);
                    WinMain2(w2, hInstance, SW_SHOW);
                    windowIndex += 1;
                }
            }
            break;
        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);

            hdcMem = CreateCompatibleDC(hdc);
            oldBitmap = SelectObject(hdcMem, hBitmap);

            GetObject(hBitmap, sizeof(bitmap), &bitmap);
            BitBlt(hdc, 100, 90, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, oldBitmap);
            DeleteDC(hdcMem);

            EndPaint(hwnd, &ps);

            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WinMain2(WNDCLASSEX wincl_2, HINSTANCE hInstance2, int nCmdShow){
    HWND hwnd2;
    char windowName[100];

    sprintf(windowName, "%s%d", g_szClassName2, windowIndex);

    ZeroMemory(&wincl_2,sizeof(WNDCLASSEX));

    wincl_2.hInstance       = hInstance2;
    wincl_2.lpszClassName   = windowName;
    wincl_2.lpfnWndProc     = (WNDPROC)WndProc2;     
    wincl_2.style           = CS_DBLCLKS;                 
    wincl_2.cbSize          = sizeof(WNDCLASSEX);
    wincl_2.hIcon           = LoadIcon(NULL, IDI_APPLICATION);
    wincl_2.hIconSm         = LoadIcon(NULL, IDI_APPLICATION);
    wincl_2.hCursor         = LoadCursor(NULL, IDC_ARROW);
    wincl_2.lpszMenuName    = NULL;                 
    wincl_2.cbClsExtra      = 0;                      
    wincl_2.cbWndExtra      = 0;                      
    wincl_2.hbrBackground   = (HBRUSH)COLOR_BACKGROUND;

    if(!RegisterClassEx(&wincl_2)){
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd2 = CreateWindowEx(
        0,                   
        windowName,        
        "Serie",     
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,      
        500, 500,                
        HWND_DESKTOP, NULL, hInstance2, NULL               
    );

    if(hwnd2 == NULL){
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd2, nCmdShow);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
    WNDCLASSEX wc;
    HWND hwnd;
    HWND hwndButton;
    MSG Msg;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style          = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc)){
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "Manga Livre",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
        NULL, NULL, hInstance, NULL
    );

    if(hwnd == NULL){
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while(GetMessage(&Msg, NULL, 0, 0) > 0){
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    
    return Msg.wParam;
}

void ShowError(char *message){
    printf("%s\n", message);
    DWORD errorCode = GetLastError();
    LPSTR messageBuffer = NULL;
    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errorCode,
        0,
        (LPTSTR)&messageBuffer,
        0,
        NULL
    );
    printf("%s\n", messageBuffer);
}

void add_elements(HWND hwnd, HINSTANCE hInstance){
    HWND hWndComboBoxType; 
    HWND hWndComboBoxPeriod;
    TCHAR A[16];

    CreateWindowExW(0L, L"BUTTON", L"Load", BS_TEXT | WS_CHILD | WS_VISIBLE, 200, 40, 100, 40, hwnd, (HMENU)0, hInstance, NULL);
    hWndComboBoxType = CreateWindowEx(0L, "COMBOBOX", "type", CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, 100, 50, 90, 300, hwnd, (HMENU)11, hInstance, NULL);
    hWndComboBoxPeriod = CreateWindowEx(0L, "COMBOBOX", "period", CBS_DROPDOWN | WS_CHILD | WS_VISIBLE, 310, 50, 90, 300, hwnd, (HMENU)12, hInstance, NULL);

    for (int k = 0; k <= 3; k++){
        wcscpy_s(A, sizeof(A)/sizeof(TCHAR), (TCHAR*)types[k]);
        SendMessage(hWndComboBoxType, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
    }

    for (int k = 0; k <= 3; k++){
        wcscpy_s(A, sizeof(A)/sizeof(TCHAR), (TCHAR*)periods[k]);
        SendMessage(hWndComboBoxPeriod, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
    }

    SendMessage(hWndComboBoxType, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
    SendMessage(hWndComboBoxPeriod, CB_SETCURSEL, (WPARAM)1, (LPARAM)0);
}

void add_buttons(HWND hwnd, HINSTANCE hInstance){
    CreateWindowExW(0L, L"BUTTON", L"<", BS_TEXT | WS_CHILD | WS_VISIBLE, 40, 140, 50, 200, hwnd, (HMENU)1, hInstance, NULL);
    CreateWindowExW(0L, L"BUTTON", L">", BS_TEXT | WS_CHILD | WS_VISIBLE, 410, 140, 50, 200, hwnd, (HMENU)2, hInstance, NULL);
    CreateWindowExW(0L, L"BUTTON", L"View", BS_TEXT | WS_CHILD | WS_VISIBLE, 200, 400, 100, 40, hwnd, (HMENU)3, hInstance, NULL);
}

const char* OpenFileDialog(HWND hwnd){
    OPENFILENAME ofn;
    char szFile[260];
    HANDLE hf;

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)==TRUE){
        return ofn.lpstrFile;
    }
}

HBITMAP ShowImage(HWND hwnd){
    const char* fileName;
    wchar_t wfileName[100];
    size_t fileSize;
    static HBITMAP hBitmap;

    //fileName = OpenFileDialog(hwnd);
    fileName = "serie.bmp";
    fileSize = strlen(fileName);
        //wfileName = malloc(fileSize+2);
        //if (!wfileName){
        //    printf("malloc failed");
        //}
        //mbstowcs_s(NULL,wfileName,strlen(fileName)+1,fileName,strlen(fileName));
    mbstowcs(wfileName, fileName, fileSize+1);
        //mbstowcs_s(&fileSize, wfileName, fileName, fileSize);
        //printf("%ls", wfileName[0]);
    hBitmap = (HBITMAP) LoadImageW(NULL, wfileName, IMAGE_BITMAP, 300, 300, LR_LOADFROMFILE);
        //free(wfileName);
    if (hBitmap == NULL) {
        ShowError("bitmap null");
    }

    return hBitmap;
}

int convert_to_bmp(char *sourcefilename, char *destfilename){
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    BMPHEAD bh;
    FILE * bmpfile;
    unsigned char *raw_image = NULL;
    unsigned long location = 0;
    int width;
    int height;
    int bytes_per_pixel;
    int color_space;
    int bytesPerLine, line, x;
    char *linebuf;
    int i = 0;
    
    FILE *infile = fopen(sourcefilename, "rb");
    
    if (!infile){
        return -1;
    }
    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);

    width=cinfo.image_width;
    height=cinfo.image_height;
    bytes_per_pixel = cinfo.num_components;

    jpeg_start_decompress(&cinfo);

    raw_image = (unsigned char*)malloc( cinfo.output_width*cinfo.output_height*cinfo.num_components );
    row_pointer[0] = (unsigned char *)malloc( cinfo.output_width*cinfo.num_components );
    
    while( cinfo.output_scanline < cinfo.image_height ){
        jpeg_read_scanlines( &cinfo, row_pointer, 1 );
        for( i=0; i<cinfo.image_width*cinfo.num_components;i++){
            raw_image[location++] = row_pointer[0][i];
        }
    }
    
    jpeg_finish_decompress( &cinfo );
    jpeg_destroy_decompress( &cinfo );
    free( row_pointer[0] );
    fclose( infile );

    memset ((char *)&bh,0,sizeof(BMPHEAD));

    bh.headersize  = 54L;
    bh.infoSize  =  0x28L;
    bh.width     = width ;
    bh.depth     = height;
    bh.biPlanes  =  1 ;
    bh.bits      = 24 ;
    bh.biCompression = 0L;;

    bytesPerLine = width * 3;
    
    if (bytesPerLine & 0x0003)
        {
            bytesPerLine |= 0x0003;
            ++bytesPerLine;
        }

    bh.filesize=bh.headersize+(long)bytesPerLine*bh.depth;

    bmpfile = fopen(destfilename, "wb");
    if (bmpfile == NULL){
        exit (1);
    }
    fwrite("BM",1,2,bmpfile);
    fwrite((char *)&bh, 1, sizeof (bh), bmpfile);

    linebuf = (char *) calloc(1, bytesPerLine);
    if (linebuf == NULL){
        free(raw_image);
        exit (1);   
    }

    for (line = height-1; line >= 0; line --){
        for( x =0 ; x < width; x++ ){
            *(linebuf+x*bytes_per_pixel) = *(raw_image+(x+line*width)*bytes_per_pixel+2);
            *(linebuf+x*bytes_per_pixel+1) = *(raw_image+(x+line*width)*bytes_per_pixel+1);
            *(linebuf+x*bytes_per_pixel+2) = *(raw_image+(x+line*width)*bytes_per_pixel+0);
        }
        fwrite(linebuf, 1, bytesPerLine, bmpfile);
    }

    free(linebuf);
    fclose(bmpfile);

    return 1;
}

static size_t cb(void *data, size_t size, size_t nmemb, void *clientp){
    size_t realsize = size * nmemb;
    struct memory *mem = (struct memory *)clientp;
    
    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if(ptr == NULL)
        return 0;
    
    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;
    
    return realsize;
}

size_t write_data_to_file(void *ptr, size_t size, size_t nmemb, FILE *stream){
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int DownloadImage(const char *url){
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char outfilename[] = "serie.jpg";
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(outfilename,"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_to_file);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
        curl_easy_setopt(curl, CURLOPT_CAPATH, "cacert.pem");
        res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    return 1;
}