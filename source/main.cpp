// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <switch.h>
#include <dirent.h>

#include "EpicGamesDAuthManager.h"
#include "UE4CommandLineManager.h"

// Include the main libnx system header, for Switch development
#include <switch.h>
#include <unistd.h>
#define TRACE(fmt, ...)                                          \
    printf("%s: " fmt "\n", __PRETTY_FUNCTION__, ##__VA_ARGS__); \
    consoleUpdate(NULL);

bool HasConnection()
{
    NifmInternetConnectionStatus status;

    nifmGetInternetConnectionStatus(nullptr, nullptr, &status);

    return status == NifmInternetConnectionStatus_Connected;
}

void printDialog(bool actionCancelled, string message = "") {
    consoleClear();
    if (actionCancelled)
    {
        printf("Action cancelled.\n\n");
    }

    if (message != "")
    {
        printf("%s\n\n", message.c_str());
    }

    printf("-----------------FORTNITE S13 LAUNCHER-----------------\nMade by thisisanto <3\n\n");

    printf("A - Manage your account\nB - Launch Fortnite\nX - Restore CommandLine arguments\n+ - Exit\n\n");
    consoleUpdate(NULL);
}


int main(int argc, char* argv[])
{
    consoleInit(NULL);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);


    PadState pad;
    padInitializeDefault(&pad);

    socketInitializeDefault();
    nifmInitialize(NifmServiceType_User);

    if (!HasConnection())
    {
        printf("You need an internet connection to use this launcher.\n");
        printf("Press + to exit.\n");
        while (appletMainLoop())
        {
            padUpdate(&pad);
            u64 kDown = padGetButtonsDown(&pad);
            if (kDown & HidNpadButton_Plus)
                break;
            consoleUpdate(NULL);
        }
        socketExit();
        nifmExit();
        consoleExit(NULL);
        return 0;
    }
    DIR *dir = opendir("sdmc:/switch/S13Launcher");
    if(!dir) {
        mkdir("sdmc:/switch/S13Launcher", 0777);
    }
    closedir(dir);

    FILE *file = fopen("sdmc:/atmosphere/contents/010025400AECE000/romfs/UE4CommandLine.txt", "r");

    if (!file) {
        fclose(file);
        file = fopen("sdmc:/atmosphere/contents/010025400AECE000/romfs/UE4CommandLine.txt", "w");
        fprintf(file, "../../../FortniteGame/FortniteGame.uproject -skippatchcheck");
    }
    fclose(file);

    printDialog(false);

    while (appletMainLoop())
    {
        padUpdate(&pad);

        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break; 

        if (kDown & HidNpadButton_A) {
            json dauth = GetDAuth();

            if(dauth.empty()) {
                printf("You're not authenticated to the Epic Games services yet. Do you want to authenticate?\nA - Yes\nB - No\n\n");
                printf("NOTE: This will require you to have access to another device with an internet connection.\n\n");
                consoleUpdate(NULL);
                while (appletMainLoop())
                {
                    padUpdate(&pad);

                    u64 kDown = padGetButtonsDown(&pad);

                    if (kDown & HidNpadButton_Plus)
                        break; 

                    if (kDown & HidNpadButton_A) {
                        InitializeAuthProcess();
                        printDialog(false);
                        break;
                    }

                    if (kDown & HidNpadButton_B) {
                        consoleClear();
                        printDialog(true);
                        break;
                    }

                    consoleUpdate(NULL);
                }
            } else {
                printf("You're currently authenticated as %s.\n\n", dauth["displayName"].get<std::string>().c_str());
                printf("A - Reauthenticate\nB - Delete Credentials\n+ - Exit\n\n");
                consoleUpdate(NULL);
                while (appletMainLoop())
                {
                    padUpdate(&pad);

                    u64 kDown = padGetButtonsDown(&pad);

                    if (kDown & HidNpadButton_Plus)
                        break; 

                    if (kDown & HidNpadButton_A) {
                        InitializeAuthProcess();
                        printDialog(false);
                        break;
                    }

                    if (kDown & HidNpadButton_B) {
                        printf("Are you sure you want to delete your credentials?\nA - Yes\nB - No\n\n");

                        while (appletMainLoop())
                        {
                            padUpdate(&pad);

                            u64 kDown = padGetButtonsDown(&pad);

                            if (kDown & HidNpadButton_Plus)
                                break; 

                            if (kDown & HidNpadButton_A) {
                                DeleteDAuth();
                                printDialog(false, "Credentials deleted.");
                                break;
                            }

                            if (kDown & HidNpadButton_B) {
                                consoleClear();
                                printDialog(true);
                                break;
                            }

                            consoleUpdate(NULL);
                        }
                        break;
                    }

                    consoleUpdate(NULL);
                }
            }
        }

        if (kDown & HidNpadButton_B) {
            json dauth = GetDAuth();
            if (dauth.empty()) {
                printDialog(false, "You need to authenticate first.");
                continue;
            }

            std::unordered_map<string, string> arguments = ParseUE4CommandLine("sdmc:/atmosphere/contents/010025400AECE000/romfs/UE4CommandLine.txt");

            if (arguments["AUTH_TYPE"] != "exchangecode") 
                storeOldUE4CommandLine(arguments);

            printf("Authenticating with Epic Games services...\n\n");
            consoleUpdate(NULL);
            std::string exchangeCode = getExchangeCode(dauth);

            if (exchangeCode == "INVALID_DEVICE_AUTH") {
                printDialog(false, "Your credentials are invalid. Please reauthenticate.");
                continue;
            }

            arguments["AUTH_PASSWORD"] = exchangeCode;
            arguments["AUTH_LOGIN"] = "unused";
            arguments["AUTH_TYPE"] = "exchangecode";
            arguments["AuthClient"] = "3f69e56c7649492c8cc29f1af08a8a12";
            arguments["AuthSecret"] = "b51ee9cb12234f50a69efa67ef53812e";

            std::string commandLine = RebuildUE4CommandLine(arguments);

            SaveUE4CommandLine(commandLine);

            printf("Launching Fortnite, you will now be prompted to select a user...\n\n");

            consoleUpdate(NULL);

            sleep(2);

            appletRequestLaunchApplication(0x010025400AECE000, NULL);
        }

        if (kDown & HidNpadButton_X) {
            std::unordered_map<string, string> arguments = ParseUE4CommandLine("sdmc:/switch/S13Launcher/OldCommandLine.txt");

            if (arguments["failedtoopen"].empty())
            {

                SaveUE4CommandLine(RebuildUE4CommandLine(arguments));

                remove("sdmc:/switch/S13Launcher/OldCommandLine.txt");

                printDialog(false, "CommandLine arguments restored.");
            }
            else
            {
                printDialog(false, "You don't have any old CommandLine arguments to restore.");
            }

        }

        consoleUpdate(NULL);
    }


    socketExit();
    nifmExit();
    consoleExit(NULL);
    return 0;
}
