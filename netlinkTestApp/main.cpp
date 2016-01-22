extern "C" {
#include <unistd.h>
}
#include "NetlinkListener.h"

int main(void)
{
	NetlinkListener * nll = new NetlinkListener();

	nll->startListening(5000);

	while (1)
	{
		sleep(1);
	}
	
	delete nll;
  	return 0;
}
