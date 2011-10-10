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

/**
 * \brief Test file
 */

function runTest () {

  Components.utils.import("resource://app/jsmodules/sbTimeFormatter.jsm");

  assertEqual(TimeFormatter.formatSingle(1), "1 second");
  assertEqual(TimeFormatter.formatSingle(2), "2 seconds");
  assertEqual(TimeFormatter.formatSingle(60), "1 minute");
  assertEqual(TimeFormatter.formatSingle(65), "1 minute");
  assertEqual(TimeFormatter.formatSingle(67), "1.1 minutes");
  assertEqual(TimeFormatter.formatSingle(3600), "1 hour");
  assertEqual(TimeFormatter.formatSingle(3959), "1 hour");
  assertEqual(TimeFormatter.formatSingle(3961), "1.1 hours");
}

