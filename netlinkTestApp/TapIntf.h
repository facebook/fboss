

class TapIntf {
	public:
	TapIntf();
	~TapIntf();

	/* Get a packet */
	int receive(const char ** pkt);

	/* Put a packet */
	void send(const char * pkt, int size);

	private:
	/* class-local variables */

	/* Disallow copy */
	TapIntf(const TapIntf &);
	TapIntf& operator=(const TapInf &);


};
