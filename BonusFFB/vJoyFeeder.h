/*
This file is part of Bonus FFB.

Bonus FFB is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

Bonus FFB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Bonus FFB. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#ifndef __wtypes_h__
#include <wtypes.h>
#endif

#ifndef __WINDEF_
#include <windef.h>
#endif

#include <QObject>
#include "public.h"
#include "vjoyinterface.h"


class vJoyFeeder : public QObject
{
	Q_OBJECT

public:
	static bool isDriverEnabled();
	static bool checkVersionMatch();
	static int deviceCount();
	bool acquire();
	bool is_acquired();
	void release();

	void pressButton(int);
	void releaseButton(int);
public slots:
	void setDeviceIndex(unsigned int);
	void updateButtons(int);
private:
	unsigned int deviceNum = 1;
	bool acquired = false;
	int pressedButton = 0;
};

