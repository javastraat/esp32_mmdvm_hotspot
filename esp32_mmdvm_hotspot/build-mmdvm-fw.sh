#!/bin/bash

#   Copyright (C) 2017,2018,2019,2020 by Andy Uribe CA6JAU

#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

echo "******************************************************"
echo "********* Cleaning objects and updating code *********"
echo "******************************************************"
cd ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/
make clean
git pull

# Building ZUMspot Libre Kit
echo "*******************************************************"
echo "********* Building ZUMspot Libre Kit firmware *********"
echo "*******************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/ZUMspot_Libre.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4 bl
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1bl.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/zumspot_libre_fw.bin
make clean

# Building ZUMspot RPi
echo "*************************************************"
echo "********* Building ZUMspot RPi firmware *********"
echo "*************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/ZUMspot_RPi.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/zumspot_rpi_fw.bin
make clean

# Building ZUMspot USB
echo "*************************************************"
echo "********* Building ZUMspot USB firmware *********"
echo "*************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/ZUMspot_USB.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4 bl
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1bl.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/zumspot_usb_fw.bin
make clean

# Building ZUMspot Duplex
echo "****************************************************"
echo "********* Building ZUMspot Duplex firmware *********"
echo "****************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/ZUMspot_duplex.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/zumspot_duplex_fw.bin
make clean

# Building ZUMspot Dualband
echo "******************************************************"
echo "********* Building ZUMspot Dualband firmware *********"
echo "******************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/ZUMspot_dualband.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/zumspot_dualband_fw.bin
make clean

# Building MMDVM_HS_Hat
echo "**************************************************"
echo "********* Building MMDVM_HS_Hat firmware *********"
echo "**************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/MMDVM_HS_Hat.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_hs_hat_fw.bin
make clean

# Building MMDVM_HS_Hat (12.288 MHz TCXO)
echo "********************************************************************"
echo "********* Building MMDVM_HS_Hat (12.288 MHz TCXO) firmware *********"
echo "********************************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/MMDVM_HS_Hat-12mhz.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_hs_hat_fw-12mhz.bin
make clean

# Building MMDVM_HS_Dual_Hat
echo "*******************************************************"
echo "********* Building MMDVM_HS_Dual_Hat firmware *********"
echo "*******************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/MMDVM_HS_Dual_Hat.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_hs_dual_hat_fw.bin
make clean

# Building MMDVM_HS_Dual_Hat (12.288 MHz TCXO)
echo "*************************************************************************"
echo "********* Building MMDVM_HS_Dual_Hat (12.288 MHz TCXO) firmware *********"
echo "*************************************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/MMDVM_HS_Dual_Hat-12mhz.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_hs_dual_hat_fw-12mhz.bin
make clean

# Building Nano hotSPOT
echo "**************************************************"
echo "********* Building Nano hotSPOT firmware *********"
echo "**************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/Nano_hotSPOT.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/nano_hotspot_fw.bin
make clean

# Building NanoDV NPi
echo "************************************************"
echo "********* Building NanoDV NPi firmware *********"
echo "************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/NanoDV_NPi.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/nanodv_npi_fw.bin
make clean

# Building NanoDV USB
echo "************************************************"
echo "********* Building NanoDV USB firmware *********"
echo "************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/NanoDV_USB.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4 bl
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1bl.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/nanodv_usb_fw.bin
make clean

# Building D2RG MMDVM_HS
echo "***************************************************"
echo "********* Building D2RG MMDVM_HS firmware *********"
echo "***************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/D2RG_MMDVM_HS.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/d2rg_mmdvm_hs.bin
make clean

# Building Generic Simplex GPIO
echo "**********************************************************"
echo "********* Building Generic Simplex GPIO firmware *********"
echo "**********************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/generic_gpio.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/generic_gpio_fw.bin
make clean

# Building Generic Duplex GPIO
echo "*********************************************************"
echo "********* Building Generic Duplex GPIO firmware *********"
echo "*********************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/generic_duplex_gpio.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/generic_duplex_gpio_fw.bin
make clean

# Building Generic Duplex USB
echo "********************************************************"
echo "********* Building Generic Duplex USB firmware *********"
echo "********************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/generic_duplex_usb.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4 bl
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1bl.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/generic_duplex_usb_fw.bin
make clean

# Building SkyBridge RPi HS
echo "******************************************************"
echo "********* Building SkyBridge RPi HS firmware *********"
echo "******************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/SkyBridge_RPi.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/skybridge_rpi_fw.bin
make clean

# Building LoneStar USB
echo "*************************************************"
echo "********* Building LoneStar USB firmware *********"
echo "*************************************************"
cp ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/configs/LoneStar_USB.h ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/Config.h
make -j4 bl
mv ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/mmdvm_f1bl.bin ~/Software/esp32_mmdvm_hotspot/MMDVM/MMDVM_HS/bin/lonestar_usb_fw.bin
make clean

#cp ~/Software/esp32_mmdvm_hotspot/MMDVM_HS/configs/ZUMspot_Libre.h ~/Software/esp32_mmdvm_hotspot/MMDVM_HS/Config.h
