#include <napi.h>
#include <string>
#include <fstream>
#include <iostream>
#include <windows.h>
#include "include/steam_api.h"
#include "LiveSplitReader.h"

using namespace Napi;

#define fileWrite(file, value, size) file.write(reinterpret_cast<const char *>(&value), size);
#define HLSRC_SECTION_DEF_SIZE 8

#define NODENUM(env, num) Number::New(env, num)
#define NODESTR(env, str) String::New(env, (const char16_t*)str)

#pragma region SteamWorks

struct SteamInfo
{
    // State
    bool connected = false;
    int friendCount = 0;

    // Steam Interfaces
    ISteamFriends *iSteamFriends = nullptr;
    ISteamApps *iSteamApps = nullptr;
    ISteamUtils *iSteamUtils = nullptr;

    // Functions
    void UpdateInterfaces()
    {
        if (steam.iSteamFriends == nullptr)
            steam.iSteamFriends = SteamFriends();
        if (steam.iSteamApps == nullptr)
            steam.iSteamApps = SteamApps();
        if (steam.iSteamUtils == nullptr)
            steam.iSteamUtils = SteamUtils();
    }
} steam;

int nGetFriendCount()
{
    if (!steam.connected)
    {
        steam.friendCount = 0;
        return steam.friendCount;
    }

    steam.UpdateInterfaces();

    ISteamFriends *steamFriends = steam.iSteamFriends;

    int friendCount = steamFriends->GetFriendCount(k_EFriendFlagAll);
    steam.friendCount = friendCount;

    return steam.friendCount;
}

Object FriendGameInfo(Env env, FriendGameInfo_t *info)
{
    Object obj = Object::New(env);

    obj.Set("m_gameID", info->m_gameID.AppID());
    return obj;
}

Array GetAvatar(Env env, int avatarHandle)
{
    unsigned int avatarHeight;
    unsigned int avatarWidth;

    auto steamUtils = SteamUtils();
    steamUtils->GetImageSize(avatarHandle, &avatarWidth, &avatarHeight);

    const int bufferSize = avatarWidth * avatarHeight * 4;

    uint8 *avatar = new uint8[bufferSize];
    bool gotAvatar = steamUtils->GetImageRGBA(avatarHandle, avatar, bufferSize);

    if (gotAvatar)
    {
        Array nodeAvatar = Array::New(env, bufferSize);

        for (int i = 0; i < bufferSize; i++)
            nodeAvatar.Set(i, avatar[i]);

        return nodeAvatar;
    }

    return Array::New(env, 0);
}

Object SteamFriend(Env env, CSteamID id, bool getAvatar, int index)
{
    Napi::Object SteamFriend = Napi::Object::New(env);

    steam.UpdateInterfaces();
    ISteamFriends *steamFriends = steam.iSteamFriends;
    uint64 id64 = id.ConvertToUint64();

    SteamFriend.Set("index", index);
    SteamFriend.Set("personaName", steamFriends->GetFriendPersonaName(id));
    SteamFriend.Set("personaState", (int)steamFriends->GetFriendPersonaState(id));
    SteamFriend.Set("friendRPC", steamFriends->GetFriendRichPresence(id, "launcher"));
    SteamFriend.Set("friendRPCCount", steamFriends->GetFriendRichPresenceKeyCount(id));
    SteamFriend.Set("friendID", std::to_string(id64).c_str());

    FriendGameInfo_t gameInfo;
    bool isPlaying = steamFriends->GetFriendGamePlayed(id, &gameInfo);

    if (isPlaying)
        SteamFriend.Set("friendGameInfo", FriendGameInfo(env, &gameInfo));

    if (getAvatar)
    {
        int avatarHandle = steamFriends->GetMediumFriendAvatar(id);
        Array avatar = GetAvatar(env, avatarHandle);
        SteamFriend.Set("avatar", avatar);
    }

    return SteamFriend;
}

Napi::Boolean BIsSubscribedApp(Napi::CallbackInfo &info)
{
    Env env = info.Env();

    if (info[0].IsNumber())
    {
        if (!steam.connected)
            return Napi::Boolean::New(env, false);
        steam.UpdateInterfaces();

        return Napi::Boolean::New(env, steam.iSteamApps->BIsSubscribedApp(info[0].As<Number>().Int32Value()));
    }
    else
    {
        return Napi::Boolean::New(env, false);
    }
}

Napi::Boolean RunSteam(Napi::CallbackInfo &info)
{
    Env env = info.Env();
    bool connected = SteamAPI_Init();

    steam.connected = connected;
    return Napi::Boolean::New(env, connected);
}

Napi::Number GetFriendCount(Napi::CallbackInfo &info)
{
    Env env = info.Env();

    return NODENUM(env, nGetFriendCount());
}

Napi::Array GetFriends(Napi::CallbackInfo &info)
{
    Env env = info.Env();

    if (!steam.connected)
        return Napi::Array::New(env, 0);
    steam.UpdateInterfaces();

    ISteamFriends *steamFriends = steam.iSteamFriends;
    int friendCount = nGetFriendCount();

    Napi::Array friends = Napi::Array::New(env, friendCount);

    for (int i = 0; i < friendCount; i++)
    {
        CSteamID id = steamFriends->GetFriendByIndex(i, k_EFriendFlagAll);
        friends.Set(i, SteamFriend(env, id, false, i));
    }

    return friends;
}

Napi::Object NGetDiskFreeSpace(Napi::CallbackInfo &info)
{
    Env env = info.Env();

    BOOL fResult;
    __int64 lpFreeBytesAvailable, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes;

    fResult = GetDiskFreeSpaceExW((const wchar_t*)info[0].As<String>().Utf16Value().c_str(), (PULARGE_INTEGER)&lpFreeBytesAvailable, (PULARGE_INTEGER)&lpTotalNumberOfBytes, (PULARGE_INTEGER)&lpTotalNumberOfFreeBytes);

    Object retVal = Object::New(env);

    retVal.Set("Result", fResult);

    if (fResult)
    {
        retVal.Set("FreeBytesAvailable", lpFreeBytesAvailable / (1024 * 1024));
        retVal.Set("TotalNumberOfBytes", lpTotalNumberOfBytes / (1024 * 1024));
        retVal.Set("TotalNumberOfFreeBytes", lpTotalNumberOfFreeBytes / (1024 * 1024));
    }

    return retVal;
}

Napi::Object ReadSplitsFile(Napi::CallbackInfo &info)
{
    Env env = info.Env();
    const wchar_t* path = (const wchar_t*)info[0].As<Napi::String>().Utf16Value().c_str();
    LiveSplitReader reader(path);

    Object retVal = Object::New(env);
    
    retVal.Set("parseStatus", (int)reader.m_result.status);
    retVal.Set("liveSplitStatus", (int)reader.m_livesplitResult);
	
	if (reader.m_livesplitResult == LiveSplitFileStatus::OK)
	{
		Object splits = Object::New(env);
		splits.Set("gameName", NODESTR(env, reader.m_splits.gameName));
		splits.Set("categoryName", NODESTR(env, reader.m_splits.categoryName));
		splits.Set("attemptCount", NODENUM(env, reader.m_splits.attemptCount));
		splits.Set("segmentCount", reader.m_splits.segments.size());
		
		if (reader.m_livesplitResult != LiveSplitFileStatus::NO_SEGMENTS)
		{
			Array segments = Array::New(env, reader.m_splits.segments.size());
			for (int i = 0; i < reader.m_splits.segments.size(); i++)
			{
				Object segment = Object::New(env);

				segment.Set("name", NODESTR(env, reader.m_splits.segments[i]->name));
				segment.Set("realTime", NODESTR(env, reader.m_splits.segments[i]->realTime));
				segment.Set("gameTime", NODESTR(env, reader.m_splits.segments[i]->gameTime));
				segment.Set("bestRealTime", NODESTR(env, reader.m_splits.segments[i]->bestRealTime));
				segment.Set("bestGameTime", NODESTR(env, reader.m_splits.segments[i]->bestGameTime));
				
				segments.Set(i, segment);
			}
			
			splits.Set("segments", segments);
		}
		
		retVal.Set("splitsInfo", splits);
	}
    
    return retVal;
}

Napi::Boolean SetRichPresence(Napi::CallbackInfo &info)
{
    Env env = info.Env();

    if (!steam.connected || info.Length() < 2)
        return Napi::Boolean::New(env, false);
    steam.UpdateInterfaces();

    ISteamFriends *steamFriends = steam.iSteamFriends;

    bool success = steamFriends->SetRichPresence(info[0].As<String>().Utf8Value().c_str(), info[1].As<String>().Utf8Value().c_str());

    return Boolean::New(env, success);
}

Napi::Object GetFriendByIndex(Napi::CallbackInfo &info)
{
    Env env = info.Env();

    if (!steam.connected)
        return Napi::Array::New(env, 0);
    steam.UpdateInterfaces();

    ISteamFriends *steamFriends = steam.iSteamFriends;

    int index = info[0].As<Number>().Int32Value();
    CSteamID id = steamFriends->GetFriendByIndex(index, k_EFriendFlagAll);

    return SteamFriend(env, id, false, index);
}

Napi::String GetPersonaName(Napi::CallbackInfo &info)
{
    Env env = info.Env();

    if (!steam.connected)
        return Napi::String::New(env, "");
    steam.UpdateInterfaces();

    ISteamFriends *steamFriends = steam.iSteamFriends;

    return Napi::String::New(env, steamFriends->GetPersonaName());
}

Napi::String GetIPCountry(Napi::CallbackInfo &info)
{
    Env env = info.Env();

    if (!steam.connected)
        return Napi::String::New(env, "");
    steam.UpdateInterfaces();

    ISteamUtils *steamUtils = steam.iSteamUtils;

    return Napi::String::New(env, steamUtils->GetIPCountry());
}

#pragma endregion

// Initialization

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    Object Steamworks = Object::New(env);
    Steamworks.Set("SteamAPI_Init", Napi::Function::New(env, RunSteam));
    Steamworks.Set("GetFriendCount", Napi::Function::New(env, GetFriendCount));
    Steamworks.Set("GetFriends", Napi::Function::New(env, GetFriends));
    Steamworks.Set("GetFriendByIndex", Napi::Function::New(env, GetFriendByIndex));
    Steamworks.Set("BIsSubscribedApp", Napi::Function::New(env, BIsSubscribedApp));
    Steamworks.Set("SetRichPresence", Napi::Function::New(env, SetRichPresence));
    Steamworks.Set("GetPersonaName", Napi::Function::New(env, GetPersonaName));
    Steamworks.Set("GetIPCountry", Napi::Function::New(env, GetIPCountry));

    exports.Set("steamworks", Steamworks);

    Object WinApi = Object::New(env);
    WinApi.Set("GetDiskFreeSpaceMbytes", Napi::Function::New(env, NGetDiskFreeSpace));

    exports.Set("WinApi", WinApi);
	
	Object LiveSplit = Object::New(env);
    LiveSplit.Set("ReadSplitsFile", Napi::Function::New(env, ReadSplitsFile));
	
    exports.Set("LiveSplit", LiveSplit);

    return exports;
}

NODE_API_MODULE(addon, Init);