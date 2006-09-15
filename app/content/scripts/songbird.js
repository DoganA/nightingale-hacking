/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright? 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the ?GPL?).
// 
// Software distributed under the License is distributed 
// on an ?AS IS? basis, WITHOUT WARRANTY OF ANY KIND, either 
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



// Make this an honest global.
var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
              .getService(Components.interfaces.sbIPlaylistPlayback);



//
// Called on init of songbird.xul.
//
function SBAppStartup()
{
  dump("SBAppStartup\n");
  try
  {
    // Gets called from doEULA, if user rejects EULA
    var doShutdown = function() {
      try {
      var nsIMetrics = new Components.Constructor("@songbirdnest.com/Songbird/Metrics;1", "sbIMetrics");
      var MetricsService = new nsIMetrics();
      MetricsService.setSessionFlag(false); // mark this session as clean, we did not crash

      // get the startup service and tell it to shut us down
      var as = Components.classes["@mozilla.org/toolkit/app-startup;1"]
              .getService(Components.interfaces.nsIAppStartup);
      if (as) {
        // do NOT replace '_' with '.', or it will be handled as metrics
        //    data: it would be posted to the metrics aggregator, then reset
        //    to 0 automatically
        // do not count next startup in metrics, since we counted this one, but it was aborted
        SBDataSetBoolValue("metrics_ignorenextstartup", true);
        //gPrefs.setBoolPref("songbird.metrics_ignorenextstartup", true);
        as.quit(as.eAttemptQuit);
      }
      } catch (err) {
        SB_LOG("doShutdown", "" + err);
      }
    };

    // Show EULA, then the first run (if accepted) or exit (if rejected)
    if ( doEULA( doFirstRun, doShutdown ) ) {
      doMainwinStart();
    }
  }
  catch ( err )
  {
    SB_LOG( "App Init", "" + err );
  }
}




//
// Launch the main window.
//
// Note:  This used to be a closure in songbird.xul
//
function doMainwinStart() 
{
  dump("doMainwinStart\n");

  //
  // Stupid pet tricks for when the user loads this page into his mainwin browser.  Duh.
  //
  var watcher = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                .getService(Components.interfaces.nsIWindowWatcher);
  var windows = watcher.getWindowEnumerator();
  var pop = true;
  while ( windows.hasMoreElements() )
  {
    var win = windows.getNext()
    win = win.QueryInterface(Components.interfaces.nsIDOMWindow);
    switch (win.name)
    {
      case "mainwin":
        pop = false;
        document.__dont_hide_rmpdemo_window = true;  // wtf?
        win.window.focus();
        break;
    }
  }
  
  if ( pop )
  {
    try
    {
      var nsIMetrics = new Components.Constructor("@songbirdnest.com/Songbird/Metrics;1", "sbIMetrics");
      var MetricsService = new nsIMetrics();
      if (MetricsService.getSessionFlag()) {
        //SB_LOG("previous session did not terminate properly!");
        metrics_inc("player", "crash");
      }
      MetricsService.setSessionFlag(true);
      MetricsService.checkUploadMetrics();
    }
    catch ( err )
    {
      SB_LOG( "App Init - Metrics - ", "" + err );
    }
    // Get mainwin URL
    var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
    var mainwin = "";
    try {
      // XXXredfive - probably can go ahead and make this a required pref
      mainwin = prefs.getCharPref("songbird.general.bones.selectedMainWinURL", mainwin);  
    } catch (err) {}

    if (!mainwin) {
      mainwin = "chrome://rubberducky/content/xul/mainwin.xul";

      // save it for later restarts and down the road if we want to allow file->new window
      prefs.setCharPref("songbird.general.bones.selectedMainWinURL", mainwin);  
    }

    window.open( mainwin,
                 "mainwin",
                 "chrome,modal=no,toolbar=no,popup=no,titlebar=no" );
  }
  else
  {
    document.defaultView.close(); // Nuke this window.  Don't open a mainwin.
  }
}






// doEULA
//
// If it has not already been accepted, show the EULA and ask the user to
//   accept. If it is not accepted we call or eval the aCancelAction, if
//   it is accepted we call/eval aAcceptAction. The processing of the
//   actions happens in a different scope (eula.xul) so the best way
//   is to pass functions in that get called, instead of js.
// returns false if the main window should not be opened as we are showing
//   the EULA and awaitng acceptance by the user
// returns true if the EULA has already been accepted previously
function doEULA(aAcceptAction, aCancelAction)
{
  dump("doEULA\n");

  //SB_LOG("doEULA");
  // set to false just to be cautious
  var retval = false;
  try { 
    // setup the callbacks
    var eulaData = new Object();
    eulaData.acceptAction = aAcceptAction;
    eulaData.cancelAction = aCancelAction;

    var eulaCheck;
    try {
      eulaCheck = gPrefs.getBoolPref("songbird.eulacheck");
    } catch (err) { /* prefs throws an exepction if the pref is not there */ }

    if ( !eulaCheck ) {
      window.openDialog( "chrome://songbird/content/xul/eula.xul",
                         "eula",
                         "chrome,centerscreen,modal=no,titlebar=yes",
                         eulaData );

      // We do not want to open the main window until we know EULA is accepted
      return false;
    }
    // Eula has been previously accepted, move along, move along.
    retval = true;  // if no accept action, just return true
    if (aAcceptAction) {
      if (typeof(aAcceptAction) == "function")
        retval = aAcceptAction();
      else
        retval = eval(aAcceptAction);
    }
  } catch (err) {
    SB_LOG("doEula", "" + err);
  }
  return retval; 
}

// doFirstRun
//
// Check the pref to see if this is the first run. If so, launch the firstrun
//   dialog and return. The handling in the firstrun dialog will cause the
//   main window to be launched.
// returns true to indicate the window should be launched
// returns false to indicate that the window should not be launched yet as the
//   firstrun dialog has been launched asynchronously and will launch the
//   main window on exit.
function doFirstRun()
{
  dump("doFirstRun\n");
    
  try {
    var haveRun;
    try {
      haveRun = gPrefs.getBoolPref("songbird.firstruncheck");
    } catch (err) { /* prefs throws an exepction if the pref is not there */ }

    if ( ! haveRun ) {
      var data = new Object();
      
      data.onComplete = doMainwinStart;
      data.document = document;

      // This cannot be modal it will block the download of extensions
      window.openDialog( "chrome://songbird/content/xul/firstrun.xul",
                         "firstrun", 
                         "chrome,centerscreen,titlebar=no,resizable=no,modal=no",
                         data );

      // Do not open main window until the non-modal first run dialog returns
      return false;
    }
  } catch (err) {
    SB_LOG("doFirstRun", "" + err);
  }

  // If we reach this point this is not the first run and the user has accepted
  //   the EULA so launch the main window.
  return true;
}



// Help
function onHelp()
{
  alert( "Aieeeeee, ayudame!" );
}




function restartApp()
{
  var nsIMetrics = new Components.Constructor("@songbirdnest.com/Songbird/Metrics;1", "sbIMetrics");
  var MetricsService = new nsIMetrics();
  MetricsService.setSessionFlag(false); // mark this session as clean, we did not crash
  var as = Components.classes["@mozilla.org/toolkit/app-startup;1"].getService(Components.interfaces.nsIAppStartup);
  if (as)
  {
    // do NOT replace '_' with '.', or it will be handled as a metrics data: it would be posted to the metrics aggregator, then reset to 0 automatically
    SBDataSetBoolValue("metrics_ignorenextstartup", true);
    const V_RESTART = 16;
    const V_ATTEMPT = 2;
    as.quit(V_RESTART);
    as.quit(V_ATTEMPT);
  }
  onExit();
}




var songbird_playURL;
function SBUrlChanged( value )
{
  if (!coreInitialCloakDone) return;
  try
  {
    var windowCloak = Components.classes["@songbirdnest.com/Songbird/WindowCloak;1"];
    if (windowCloak) {
      var service = windowCloak.getService(Components.interfaces.sbIWindowCloak);
      if (service) {
        // value _should_ be set correctly now.
        if (value == null)
          value = SBDataGetStringValue("faceplate.play.url");
        if ( gPPS.isVideoURL( value ) ) {
          service.uncloak( document );
          window.focus(); 
        }
        else {
          service.cloak( document ); 
        }
      }
    }
  }
  catch(e)
  {
    dump(e);
  }
}

function SBMetricsAppShutdown() 
{
  var startstamp = SBDataGetIntValue("startup_timestamp");
  var timenow = new Date();
  var diff = (timenow.getTime() - startstamp)/60000;
  metrics_add("player", "timerun", null, diff);
}



function SBAppDeinitialize()
{
  try {
    gPPS.stop(); // else we crash?
  } catch (e) {}

  // Unattach the player repeater. (please deprecate me, soon!)
  //thePlayerRepeater.unbind();
  // Unbind the playback url viewer. (used by the code that uncloaks the video window)
  songbird_playURL.unbind();
  songbird_playURL = null;
  // Remember where the video window is.
  resetGlobalHotkeys();
  onWindowSaveSizeAndPosition();
  SBMetricsAppShutdown();
}

function SBMetricsAppStart() 
{
  // do NOT replace '_' with '.', or it will be handled as a metrics data: it would be posted to the metrics aggregator, then reset to 0 automatically
  if (!SBDataGetBoolValue("metrics_ignorenextstartup"))
  {
    metrics_inc("player", "appstart", null);
  }
  // do NOT replace '_' with '.', or it will be handled as a metrics data: it would be posted to the metrics aggregator, then reset to 0 automatically
  SBDataSetBoolValue("metrics_ignorenextstartup", false);
  var timestamp = new Date();
  SBDataSetIntValue("startup_timestamp", timestamp.getTime());
}



// observer for DataRemote
const sb_url_changed = {
  observe: function ( aSubject, aTopic, aData ) { SBUrlChanged(aData); }
}

function SBAppInitialize()
{
  dump("SBAppInitialize\n");
  try
  {
    SBMetricsAppStart();

    try {
      fixOSXWindow("cheezy_window_top", "app_title");
    }
    catch (e) { }

    setVideoMinMaxCallback();
    onWindowLoadSizeAndPosition();
    createLibraryRef();
    initGlobalHotkeys();

    // Create and bind DataRemote
    songbird_playURL = SB_NewDataRemote( "faceplate.play.url", null );
    songbird_playURL.bindObserver( sb_url_changed, true );

    /*
    */
    var theWMPInstance = document.getElementById( "core_wm" );
    var theWMPBox = document.getElementById( "box_wm" );

    /*
    */
    var theQTInstance = document.getElementById( "core_qt_document" );
    var theQTBox = document.getElementById( "box_qt" );

    /*
    */
    var theVLCInstance = document.getElementById( "core_vlc" );
    var theVLCBox = document.getElementById( "box_vlc" );

    /*
    */
    var theGSTInstance = document.getElementById( "box_gstreamer_simple" );
    var theGSTBox = document.getElementById( "box_gstreamer_simple" );

    /*
    var theFLInstance = document.getElementById( "core_flash_frame" );
    var theFLBox = document.getElementById( "box_flash" );
    var theTotemInstance = document.getElementById( "core_totem_frame" );
    var theTotemBox = document.getElementById( "box_totem" );
    */
    var theGStreamerSimpleBox = document.getElementById( "box_gstreamer_simple" );

    var platform;
    try {
      var sysInfo =
        Components.classes["@mozilla.org/system-info;1"]
                  .getService(Components.interfaces.nsIPropertyBag2);
      platform = sysInfo.getProperty("name");                                          
    }
    catch (e) {
      dump("System-info not available, trying the user agent string.\n");
      var user_agent = navigator.userAgent;
      if (user_agent.indexOf("Windows") != -1)
        platform = "Windows_NT";
      else if (user_agent.indexOf("Mac OS X") != -1)
        platform = "Darwin";
      else if (user_agent.indexOf("Linux") != -1)
        platform = "Linux";
    }

    //
    // Depending upon the platform, initialize one core
    // and hide all of the rest of them.
    //
    
    if (platform == "Windows_NT") {
      //Windows, prefer VLC.

      // Initialize with VLC
      CoreVLCDocumentInit( "core_vlc" );
      //InitPlaybackCoreFlash( "core_flash_frame" );
      // Hide Quicktime
      if (theQTBox) theQTBox.hidden = true;
      // Hide GStreamer
      if (theGSTBox) theGSTBox.hidden = true;
      /*
      // Hide Flash
      if (theFLBox) theFLBox.hidden = true;
      // Hide Totem
      if (theTotemBox) theTotemBox.hidden = true;
      */
    }
    else if (platform == "Darwin") {
      //MacOSX, prefer QT.

      // Initialize with Quicktime
      CoreQTDocumentInit( "core_qt_document" );
      // Hide VLC
      if (theVLCBox) theVLCBox.hidden = true;
      // Hide GStreamer
      if (theGSTBox) theGSTBox.hidden = true;
      /*
      // Hide Flash
      if (theFLBox) theFLBox.hidden = true;
      // Hide Totem
      if (theTotemBox) theTotemBox.hidden = true;
      */
    }
    else if (platform == "Linux") {
      //Linux, prefer totem-gstreamer

      //CoreVLCDocumentInit( "core_vlc_document" );
      //InitPlaybackCoreMPlayer( "core_mp_frame" );
      //InitPlaybackCoreFlash( "core_flash_frame" );
      //CoreTotemDocumentInit( "core_totem_frame" );
      CoreGStreamerSimpleDocumentInit( "box_gstreamer_simple" );
      // Hide VLC
      if (theVLCBox) theVLCBox.hidden = true;
      // Hide Quicktime
      if (theQTBox) theQTBox.hidden = true;
      /*
      // Hide Flash
      if (theFLBox) theFLBox.hidden = true;
      */
    }
    
    // Reset this on application startup. 
    SBDataSetIntValue("backscan.paused", 0);
    
    // Go make sure we really have a Songbird database
    SBInitializeNamedDatabase( "songbird" );
/*
*/    
    try
    {
      DPUpdaterInit(1);
    }
    catch(err)
    {
      alert("DPUpdaterInit(1) - " + err);
    }
    
    try
    {
      WFInit();
    }
    catch(err)
    {
      alert("WFInit() - " + err);
    }

    // If we are a top level window, hide us.
    if ( window.parent == window )
    {
      var hide = true;
      if ( document.__dont_hide_rmpdemo_window )
      {
        hide = false;
      }
      if ( hide )
      {
        setTimeout(HideCoreWindow, 0);
      }
    }
  }
  catch( err )
  {
    alert( "SBAppInitialize\n" + err );
  }
}

function SBInitializeNamedDatabase( db_name )
{
  // This creates the a working songbird database.
  // If it already exists, it just errors silently and you don't care.
  try
  {
    // Make an async database query object for the main songbird database
    var aDBQuery = new sbIDatabaseQuery();
    aDBQuery.setAsyncQuery(true);
    aDBQuery.setDatabaseGUID(db_name);
    
    // Get the library interface, and create the library through the query
    var aMediaLibrary = new sbIMediaLibrary();    
    aMediaLibrary.setQueryObject(aDBQuery);
    aMediaLibrary.createDefaultLibrary();
    
    // Ger the playlist manager, and create the internal playlisting infrastructure.
    var aPlaylistManager = new sbIPlaylistManager();
    aPlaylistManager.createDefaultPlaylistManager(aDBQuery);
  }
  catch(err)
  {
    alert("SBInitializeNamedDatabase\n\n" + err);
  }
}

var coreInitialCloakDone = 0;
function HideCoreWindow() 
{
  onHide();
  coreInitialCloakDone = 1;
}

function onHideButtonClick()
{
  // Stop video playback
  gPPS.stop();
  // Hide our video window
  onHide();

  // And focus the main window
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var mainWin = wm.getMostRecentWindow("Songbird:Main");
  if (mainWin)
    mainWin.focus();
}

var SBVideoMinMaxCB = 
{
  // Shrink until the box doesn't match the window, then stop.
  GetMinWidth: function()
  {
    // What we'd like it to be
    var retval = 720;
    // However, if in resizing the window size is different from the document's box object
    if (window.innerWidth != document.getElementById('window_parent').boxObject.width)
    { 
      // That means we found the document's min width.  Because you can't query it directly.
      retval = document.getElementById('window_parent').boxObject.width - 1;
    }
    return retval;
  },

  GetMinHeight: function()
  {
    // What we'd like it to be
    var retval = 450;
    // However, if in resizing the window size is different from the document's box object
    if (window.innerHeight != document.getElementById('window_parent').boxObject.height)
    { 
      // That means we found the document's min width.  Because you can't query it directly.
      retval = document.getElementById('window_parent').boxObject.height - 1;
    }
    return retval;
  },

  GetMaxWidth: function()
  {
    return -1;
  },

  GetMaxHeight: function()
  {
    return -1;
  },
  
  OnWindowClose: function()
  {
    setTimeout(quitApp, 0);
  },

  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIWindowMinMaxCallback) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }
}

function setVideoMinMaxCallback()
{
  try {
    var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
    if (windowMinMax) {
      var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);
      if (service)
        service.setCallback(document, SBVideoMinMaxCB);
    }
  }
  catch (err) {
    // No component
    dump("Error. No WindowMinMax component available." + err + "\n");
  }
}

function createLibraryRef() {
 /* // this is so we can playRef the library even if it has never been shown
*/
  var source = new sbIPlaylistsource();
  source.feedPlaylist( "NC:songbird_library", "songbird", "library");
  source.executeFeed( "NC:songbird_library" );
  // Synchronous call!  Woo hoo!
  while( source.isQueryExecuting( "NC:songbird_library" ) )
    ;
  // After the call is done, force GetTargets
  source.forceGetTargets( "NC:songbird_library", false );
}




var songbird_restartNow;


const sb_restart_app = {
    observe: function ( aSubject, aTopic, aData ) { setTimeout("restartApp();", 0); }
}

function SBInterfaceInitialize() 
{
  songbird_restartNow = SB_NewDataRemote( "restart.restartnow", null );
  songbird_restartNow.bindObserver( sb_restart_app, true );
}


function SBInterfaceDeinitialize() 
{
  // Unbind restartapp remote
  songbird_restartNow.unbind();
  songbird_restartNow = null;
}
