// Juice 1.5.2a: JuiceNetwork.cpp, la interfaz de red que implementa el juego
// (JuiceDirtyNet en zMisc, de cuya tabla virtual sale el orden de los metodos
// y de cuyos mangled salen las firmas). Escrito del asm de la A124
// (JuiceNetwork.o: .text 0x4B56E8, 3 etiquetas). Unidad solo enlaza en A124.
// El constructor se registra como la extension de red en curso.

namespace Juice {

class INetwork {
public:
    INetwork();
    virtual int Connect(const char *host, int port) = 0;
    virtual int IsConnected() = 0;
    virtual int Recv(char *buf, int *channel, int size) = 0;
    virtual int PeekHdr(char *data) = 0;
    virtual int Send(int length, int channel, char *sendBuffer) = 0;
    virtual int IsEnabled() = 0;
    virtual int Initialize() = 0;
    virtual int Disconnect() = 0;
    virtual void MaintainConn() = 0;
    virtual void TearDown() = 0;
    virtual void FreeEverything() = 0;
    virtual char *GetMac() = 0;
    virtual char *GetLocalIpAddress() = 0;
    virtual ~INetwork();

    static INetwork *sCurrentNetExtention;
};

INetwork *INetwork::sCurrentNetExtention;

INetwork::INetwork() {
    sCurrentNetExtention = this;
}

INetwork::~INetwork() {}

}
