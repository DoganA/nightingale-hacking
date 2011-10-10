/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END NIGHTINGALE GPL
//
*/

// Get rid of the temp folder created by getTempFolder.
(function(){
  for each (var file in gFilesToClose) {
    try {
      file.close();
    } catch(e) {
      /* nothing */
    }
  }
})();
removeTempFolder();
(function tail_callback(){
  for each (var callback in gTailCallback) {
    try {
      callback();
    } catch (e) {
      log(e);
    }
  }
})();
