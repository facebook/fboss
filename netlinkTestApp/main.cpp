extern "C" {
#include <unistd.h>
}
#include "NetlinkListener.h"

NetlinkListener * nll = NULL;

int main(void)
{
	nll = new NetlinkListener(std::string("wedgetap"), 3);

	nll->startNetlinkListener(5000);

	while (1)
	{
		sleep(1);
	}
	
	delete nll;
  	return 0;
}
