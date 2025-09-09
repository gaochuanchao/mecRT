//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package mecRT.mecrt.common;

// store all the necessary data for VEC experiments
simple BandManager
{
    parameters:
        @display("i=block/cogwheel");

        // Carrier Frequency (GHz) -->
        double carrierFrequency @unit("GHz")= default(2GHz);

        // numerology index
        int numerologyIndex = default(0);

        @signal[offloadEnergyConsumed];
        @statistic[offloadEnergyConsumed](title="energy consumed for offloading data"; unit="mJ"; source="offloadEnergyConsumed"; record=sum,timeavg);
        
}
