/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright� 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the �GPL�).
// 
// Software distributed under the License is distributed 
// on an �AS IS� basis, WITHOUT WARRANTY OF ANY KIND, either 
// express or implied. See the GPL for the specific language 
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this 
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc., 
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
// END SONGBIRD GPL
//
 */
 
 // This needs to become an api so plugins can register new actions and callbacks for them

var hotkey_service;

var user_agent = navigator.userAgent;
var PLATFORM_WIN32 = user_agent.indexOf("Windows") != -1;
var PLATFORM_MACOSX = user_agent.indexOf("Mac OS X") != -1;
var PLATFORM_LINUX = user_agent.indexOf("Linux") != -1;

var meta_key_str = "meta";
if (PLATFORM_WIN32) meta_key_str = "win";
if (PLATFORM_MACOSX) meta_key_str = "command";
if (PLATFORM_LINUX) meta_key_str = "meta";

function initGlobalHotkeys() {
  // Access global hotkeys component (if it exists)
  var globalHotkeys = Components.classes["@songbirdnest.com/Songbird/GlobalHotkeys;1"];
  if (globalHotkeys) {
    hotkey_service = globalHotkeys.getService(Components.interfaces.sbIGlobalHotkeys);
    loadHotkeysFromPrefs()
  }
}

function resetGlobalHotkeys() {
  // Remove all hotkey bindings
  if (hotkey_service) hotkey_service.RemoveAllHotkeys();
  stopWatchingHotkeyRemotes();
}

// Hotkey handler, this gets called back when the user presses a hotkey that has been registered in loadHotkeysFromPrefs
var hotkeyHandler = 
{
    OnHotkey: function( hotkeyid )
    {
      switch ( hotkeyid.toLowerCase() )
      {
        // Call the appropriate action 
        case "playback: volume up": hotkey_volumeUp(); break;
        case "playback: volume down": hotkey_volumeDown(); break;
        case "playback: next in playlist": hotkey_nextTrack(); break;
        case "playback: previous in playlist": hotkey_previousTrack(); break;
        case "playback: play/pause": hotkey_playPause(); break;
        case "playback: pause": hotkey_pause(); break;
        default: alert("Unknown hotkey action '" + hotkeyid + "'"); break;
      }
    },

    QueryInterface : function(aIID)
    {
      if (!aIID.equals(Components.interfaces.sbIGlobalHotkeyCallback) &&
          !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
          !aIID.equals(Components.interfaces.nsISupports)) 
      {
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      
      return this;
    }
}

// Load hotkeys
function loadHotkeysFromPrefs() {
  if (!hotkey_service) return;
  beginWatchingHotkeyRemotes();
  setDefaultGlobalHotkeys();
  var enabled = SBDataGetIntValue("globalhotkeys.enabled");
  //log("(re)init - hotkeys are " + (enabled ? "enabled" : "disabled"));
  if (!enabled) return;
  var count = SBDataGetIntValue("globalhotkeys.count");
  for (var i=0;i<count;i++) {
    // Read hotkey binding from user preferences
    var root = "globalhotkey." + i + ".";
    var keycombo = SBDataGetValue(root + "key");
    var action = SBDataGetValue(root + "action");
    // Split key combination string
    var keys = keycombo.split("-");
    // Parse its components
    var alt = false;
    var ctrl = false;
    var shift = false;
    var meta = false;
    var keyCode = 0;
    for (var j=0;j<keys.length;j++) {
      keys[j] = keys[j].toLowerCase();
      if (keys[j] == "alt") alt = true;
      else if (keys[j] == "shift") shift = true;
      else if (keys[j] == "ctrl") ctrl = true;
      else if (keys[j] == "meta") meta = true;
      else keyCode = stringToKeyCode(keys[j]);
    }
    // If we had a key code (and possibly modifiers), register the corresponding action for it
    if (keyCode != 0) hotkey_service.AddHotkey(keyCode, alt, ctrl, shift, meta, action, hotkeyHandler);
  }
}

// Sets the default hotkey settings
function setDefaultGlobalHotkeys() {
  if (!SBDataGetIntValue("globalhotkeys.changed")) {
    SBDataSetValue("globalhotkeys.changed", 1);
    SBDataSetValue("globalhotkeys.enabled", true);

    SBDataSetValue("globalhotkeys.count", 5);
    
    SBDataSetValue("globalhotkey.0.key",             "meta-$38");
    SBDataSetValue("globalhotkey.0.key.readable",    meta_key_str + "-up");
    SBDataSetValue("globalhotkey.0.action",          "Playback: Volume up");
    
    SBDataSetValue("globalhotkey.1.key",             "meta-$40");
    SBDataSetValue("globalhotkey.1.key.readable",    meta_key_str + "-down");
    SBDataSetValue("globalhotkey.1.action",          "Playback: Volume down");
    
    SBDataSetValue("globalhotkey.2.key",             "meta-$39");
    SBDataSetValue("globalhotkey.2.key.readable",    meta_key_str + "-right");
    SBDataSetValue("globalhotkey.2.action",          "Playback: Next in playlist");
    
    SBDataSetValue("globalhotkey.3.key",             "meta-$37");
    SBDataSetValue("globalhotkey.3.key.readable",    meta_key_str + "-left");
    SBDataSetValue("globalhotkey.3.action",          "Playback: Previous in playlist");
    
    SBDataSetValue("globalhotkey.4.key",             "meta-$96");
    SBDataSetValue("globalhotkey.4.key.readable",    meta_key_str + "-numpad0");
    SBDataSetValue("globalhotkey.4.action",          "Playback: Play/Pause");
  }
}

// Returns the key code for a given key string
function stringToKeyCode( str ) {
  if (str.slice(0, 1) == '$') return str.slice(1);
  return 0;
}

var songbird_hotkeys_event1;
var songbird_hotkeys_event2;

function beginWatchingHotkeyRemotes() {
  songbird_hotkeys_event1 = new sbIDataRemote("globalhotkeys.changed");
  songbird_hotkeys_event1.bindCallbackFunction( globalHotkeysChanged, true )
  songbird_hotkeys_event2 = new sbIDataRemote("globalhotkeys.enabled");
  songbird_hotkeys_event2.bindCallbackFunction( globalHotkeysChanged, true )
}

function stopWatchingHotkeyRemotes() {
  songbird_hotkeys_event1.unbind();
  songbird_hotkeys_event2.unbind();
}

function globalHotkeysChanged(v) {
  resetGlobalHotkeys();
  loadHotkeysFromPrefs();
}


/*
function log(str) {
  var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                          .getService(Components.interfaces.nsIConsoleService);
  consoleService.logStringMessage(str);
}
*/

// -------------------------------------------------------------------------------------------------------------------------------------------------------------
// Hotkey Actions
// -------------------------------------------------------------------------------------------------------------------------------------------------------------

// Playback

var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                      .getService(Components.interfaces.sbIPlaylistPlayback);

function hotkey_volumeUp() {
  var volume = gPPS.getVolume() + 8;
  if (volume > 255) volume = 255;
  gPPS.setVolume(volume);
}

function hotkey_volumeDown() {
  var volume = gPPS.getVolume() - 8;
  if (volume < 0) volume = 0;
  gPPS.setVolume(volume);
}

function hotkey_nextTrack() {
  gPPS.next();
}

function hotkey_previousTrack() {
  gPPS.previous();
}

function hotkey_playPause() {
  gPPS.play(); // automatically selects play or pause depending on current state
}

function hotkey_pause() {
  gPPS.pause();
}

