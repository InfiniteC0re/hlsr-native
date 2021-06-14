const fs = require("fs");
const HLSRNative = require("./build/Release/main.node");

console.log(HLSRNative.WinApi.GetDiskFreeSpaceMbytes("D:/"))

// fs.writeFileSync("./steam_appid.txt", "250820");

// HLSRNative.steamworks.SteamAPI_Init();
// let hasHL1 = HLSRNative.steamworks.BIsSubscribedApp(70);
// let hasHL2 = HLSRNative.steamworks.BIsSubscribedApp(220);

// let friends = HLSRNative.steamworks.GetFriends();

// // friends.forEach((friend) => {
// //   if (friend.friendGameInfo) console.log(friend);
// // });
// console.log(HLSRNative.steamworks.SetRichPresence("hlsr"), hasHL1, hasHL2);

// setTimeout(() => {
//   console.log(HLSRNative.steamworks.GetFriendByIndex(62));
//   while (true) {}
// }, 3000);
