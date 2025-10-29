#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ID_LISTBOX 1001
#define ID_BUTTON_APPLY 1002
#define ID_RADIO_OLD 1003
#define ID_RADIO_64 1004

typedef struct {
    char name[100];
    char address[100];
} Server;

Server *servers = NULL;
int server_count = 0;
HWND hListBox, hButtonApply, hRadioOld, hRadio64;
int selected_version = 0;

void read_servers() {
    FILE *file = fopen("servers.json", "r");
    if (!file) {
        MessageBoxA(NULL, "Не найден файл servers.json", "Ошибка", MB_OK);
        return;
    }

    char buffer[1024];
    int capacity = 10;
    servers = malloc(capacity * sizeof(Server));
    server_count = 0;
    
    char current_name[100] = "";
    char current_address[100] = "";
    
    while (fgets(buffer, sizeof(buffer), file)) {
        char *line = buffer;
        while (*line == ' ' || *line == '\t' || *line == '\n' || *line == '\r') line++;
        
        if (strstr(line, "\"name\"")) {
            char *start = strchr(line, ':');
            if (!start) continue;
            
            start++;
            while (*start == ' ' || *start == '\t') start++;
            
            if (*start != '\"') continue;
            start++;
            
            char *end = strchr(start, '\"');
            if (!end) continue;
            
            int len = end - start;
            if (len >= sizeof(current_name)) len = sizeof(current_name) - 1;
            strncpy(current_name, start, len);
            current_name[len] = '\0';
        }
        else if (strstr(line, "\"address\"")) {
            char *start = strchr(line, ':');
            if (!start) continue;
            
            start++;
            while (*start == ' ' || *start == '\t') start++;
            
            if (*start != '\"') continue;
            start++;
            
            char *end = strchr(start, '\"');
            if (!end) continue;
            
            int len = end - start;
            if (len >= sizeof(current_address)) len = sizeof(current_address) - 1;
            strncpy(current_address, start, len);
            current_address[len] = '\0';
            
            if (strlen(current_name) > 0 && strlen(current_address) > 0) {
                if (server_count >= capacity) {
                    capacity *= 2;
                    servers = realloc(servers, capacity * sizeof(Server));
                }
                
                strcpy(servers[server_count].name, current_name);
                strcpy(servers[server_count].address, current_address);
                server_count++;
                
                current_name[0] = '\0';
                current_address[0] = '\0';
            }
        }
    }
    
    fclose(file);
}

BOOL delete_ssl_value() {
    HKEY hKey;
    LONG result;
    const char* keyPath = "Software\\MRIM\\Agent";
    
    result = RegOpenKeyExA(HKEY_CURRENT_USER, keyPath, 0, KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS) {
        return TRUE;
    }
    
    result = RegDeleteValueA(hKey, "ssl");
    RegCloseKey(hKey);
    
    return (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
}

BOOL write_registry(const char* server_address) {
    HKEY hKey;
    LONG result;
    const char* keyPath = "Software\\MRIM\\Agent";
    
    result = RegCreateKeyExA(HKEY_CURRENT_USER, keyPath, 0, NULL, 0, 
                           KEY_WRITE, NULL, &hKey, NULL);
    
    if (result != ERROR_SUCCESS) {
        return FALSE;
    }
    
    result = RegSetValueExA(hKey, "strict_server", 0, REG_SZ, 
                          (BYTE*)server_address, strlen(server_address) + 1);
    
    if (result != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return FALSE;
    }
    
    if (selected_version == 0) {
        RegDeleteValueA(hKey, "ssl");
    } else {
        const char* ssl_value = "deny";
        result = RegSetValueExA(hKey, "ssl", 0, REG_SZ, 
                              (BYTE*)ssl_value, strlen(ssl_value) + 1);
    }
    
    RegCloseKey(hKey);
    return (result == ERROR_SUCCESS);
}

void check_current_settings() {
    HKEY hKey;
    LONG result;
    const char* keyPath = "Software\\MRIM\\Agent";
    
    result = RegOpenKeyExA(HKEY_CURRENT_USER, keyPath, 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS) {
        char ssl_value[20] = "";
        DWORD size = sizeof(ssl_value);
        if (RegQueryValueExA(hKey, "ssl", NULL, NULL, (BYTE*)ssl_value, &size) == ERROR_SUCCESS) {
            selected_version = 1;
            SendMessage(hRadio64, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(hRadioOld, BM_SETCHECK, BST_UNCHECKED, 0);
        } else {
            selected_version = 0;
            SendMessage(hRadioOld, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(hRadio64, BM_SETCHECK, BST_UNCHECKED, 0);
        }
        
        RegCloseKey(hKey);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            hListBox = CreateWindowA("LISTBOX", "", 
                                   WS_CHILD | WS_VISIBLE | LBS_STANDARD,
                                   10, 10, 300, 150, 
                                   hwnd, (HMENU)ID_LISTBOX, 
                                   GetModuleHandle(NULL), NULL);
            
            hRadioOld = CreateWindowA("BUTTON", "Версия <5.X", 
                                    WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                    10, 170, 120, 20, 
                                    hwnd, (HMENU)ID_RADIO_OLD, 
                                    GetModuleHandle(NULL), NULL);
            
            hRadio64 = CreateWindowA("BUTTON", "Версия 6.4", 
                                   WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                   140, 170, 120, 20, 
                                   hwnd, (HMENU)ID_RADIO_64, 
                                   GetModuleHandle(NULL), NULL);
            
            hButtonApply = CreateWindowA("BUTTON", "Применить сервер", 
                                       WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                       10, 200, 300, 30, 
                                       hwnd, (HMENU)ID_BUTTON_APPLY, 
                                       GetModuleHandle(NULL), NULL);
            
            read_servers();
            for (int i = 0; i < server_count; i++) {
                SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)servers[i].name);
            }
            
            check_current_settings();
            break;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BUTTON_APPLY) {
                int selected = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
                if (selected != LB_ERR) {
                    if (write_registry(servers[selected].address)) {
                        char success_msg[256];
                        sprintf(success_msg, "Сервер успешно изменен на: %s\nВерсия: %s", 
                                servers[selected].address,
                                selected_version == 0 ? "<5.X" : "6.4");
                        MessageBoxA(hwnd, success_msg, "Успех", MB_OK);
                    } else {
                        MessageBoxA(hwnd, "Ошибка записи в реестр!", "Ошибка", MB_OK);
                    }
                } else {
                    MessageBoxA(hwnd, "Выберите сервер из списка!", "Ошибка", MB_OK);
                }
            }
            else if (LOWORD(wParam) == ID_RADIO_OLD) {
                selected_version = 0;
            }
            else if (LOWORD(wParam) == ID_RADIO_64) {
                selected_version = 1;
            }
            break;
            
        case WM_DESTROY:
            if (servers) free(servers);
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "ServerChangerClass";
    
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClassA(&wc);
    
    HWND hwnd = CreateWindowExA(0, CLASS_NAME, "MRIM Agent Server Changer",
                               WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
                               CW_USEDEFAULT, CW_USEDEFAULT, 350, 300,
                               NULL, NULL, hInstance, NULL);
    
    if (!hwnd) return 0;
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return msg.wParam;
}