// websocketpp
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/base64/base64.hpp>

// property_tree (json)
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

// rtmidi
#include <RtMidi.h>

// std
#include <iostream>
#include <set>

class SysexServer {
public:
  typedef websocketpp::connection_hdl connection_hdl;
  typedef websocketpp::server<websocketpp::config::asio> server;
  
  SysexServer(const std::string token, const bool debug)
  : m_debug(debug)
  , m_token(token)
  {
  }

  void run(const unsigned short listenPort)
  {
    try {
      // Set logging settings
      m_server.set_access_channels(websocketpp::log::alevel::all);
      m_server.clear_access_channels(websocketpp::log::alevel::frame_payload);
      m_server.clear_access_channels(websocketpp::log::alevel::frame_header);

      // Initialize Asio
      m_server.init_asio();

      // Register our handlers
      using websocketpp::lib::placeholders::_1;
      using websocketpp::lib::placeholders::_2;
      using websocketpp::lib::bind;
      m_server.set_message_handler(bind(&SysexServer::messageHandler, this, _1, _2));
      m_server.set_open_handler(bind(&SysexServer::openHandler, this, _1));
      m_server.set_close_handler(bind(&SysexServer::closeHandler, this, _1));

      // Listen on port 9002
      m_server.listen(listenPort);

      // Start the server accept loop
      std::cout << "sysexd: " << "listening on port " << listenPort << std::endl;
      m_server.start_accept();

      // Start the ASIO io_service run loop
      m_server.run();
    } catch (websocketpp::exception const & e) {
      std::cerr << "sysexd: " << "(exception) " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "sysexd: " << "(unhandled exception)" << std::endl;
    }
  }

public:
  // server on_open handler
  void openHandler(connection_hdl hdl) {
    if (m_debug) {
      std::cout << "sysexd: " << "accepted connection" << std::endl;
    }
    
    // Store connection
    m_connections.insert(hdl);
    std::shared_ptr<ConnectionInfo> connectionInfo = std::make_shared<ConnectionInfo>();
    
    // Register inline callbacks
    SysexInterface::errorCallback_t errorInHandler([this, hdl](const std::string message) {
      std::cerr << "sysexd: " << "(midi in error) " << message << std::endl;
      sendMessage(hdl, SysexServer::constructMessage("midierrorin", message));
    });
    SysexInterface::errorCallback_t errorOutHandler([this, hdl](const std::string message) {
      std::cerr << "sysexd: " << "(midi out error) " << message << std::endl;
      sendMessage(hdl, SysexServer::constructMessage("midierrorout", message));
    });
    SysexInterface::messageCallback_t messageHandler([this, hdl](const std::string message) {
      sendMessage(hdl, SysexServer::constructMessage("midimessage", message));
    });
    connectionInfo->getSysexInterface().setCallbacks(errorInHandler, errorOutHandler, messageHandler);
    
    // Store connection info
    connectionInfo->getSysexInterface().setDebug(m_debug);
    m_connectionInfos[hdl] = connectionInfo;
  }
  
  // Server on_close handler
  void closeHandler(connection_hdl hdl) {
    if (m_debug) {
      std::cout << "sysexd: " << "closing connection" << std::endl;
    }

    // Remove connection
    m_connectionInfos.erase(hdl);
    m_connections.erase(hdl);
  }
  
  // Constructs a JSON message using the given type and data strings
  static std::string constructMessage(const std::string &type, const std::string &data) {
    std::stringstream ss;
    boost::property_tree::ptree rootOut;

    rootOut.put("type", type);
    rootOut.put("data", data);

    boost::property_tree::write_json(ss, rootOut);
    
    return ss.str();
  }
  
  // Constructs a JSON message using the given type string and data tree
  static std::string constructMessage(const std::string &type, const boost::property_tree::ptree &data) {
    std::stringstream ss;
    boost::property_tree::ptree rootOut;

    rootOut.put("type", type);
    rootOut.add_child("data", data);

    boost::property_tree::write_json(ss, rootOut);
    
    return ss.str();
  }
  
  // Sends a message to a given connection
  void sendMessage(connection_hdl hdl, const std::string &message) {
    // Send output message as text
    try {
      m_server.send(hdl, message, websocketpp::frame::opcode::text);
    }
    catch (const websocketpp::lib::error_code& e) {
      std::cerr << "sysexd: " << "(send exception) " << e.message() << std::endl;
    }
  }

  // Server on_message handler
  void messageHandler(connection_hdl hdl, server::message_ptr messageIn) {
    try {
      boost::property_tree::ptree rootIn;
      std::string messageOut;

      // Get connection info for this connection
      const std::shared_ptr<ConnectionInfo> info = m_connectionInfos.at(hdl);

      // Parse input message (JSON)
      {
        std::stringstream ss;
        ss << messageIn->get_payload();
        boost::property_tree::read_json(ss, rootIn);
      }
      
      // Verify token
      std::string token = rootIn.get<std::string>("token");
      if (token == m_token) {
        std::string type = rootIn.get<std::string>("type");
        if (type == "query") {
          /**
           * query
           *
           * Queries MIDI interfaces and returns MIDI interface ports.
           *
           * @returns Object
           * {
           *   inports: [
           *     {
           *       name: (String) port name.
           *     }, ...
           *   ]
           *   outports: [
           *     {
           *       name: (String) port name.
           *     }, ...
           *   ]
           * }
           */
          // Construct response
          messageOut = constructMessage("query", info->getSysexInterface().getPorts());
        }
        else if (type == "inport") {
          /**
           * inport
           *
           * Sets the MIDI interface input port from which to receive messages.
           *
           * @returns Object
           * {
           *   "status": (Boolean) success or failure status.
           * }
           */
          // Indicate failure by default
          bool success = false;
          try
          {
            // Parse send parameters
            unsigned port = std::stoul(rootIn.get<std::string>("port"));
            
            // Open the given input port
            success = info->getSysexInterface().openInputPort(port);
          }
          catch (const boost::property_tree::json_parser::json_parser_error &e) {
            std::cerr << "sysexd: " << "(json parse exception) " << e.message() << std::endl;
          }
          catch (const std::invalid_argument &e) {
            std::cerr << "sysexd: " << "(invalid port)" << std::endl;
          }
          catch (const std::out_of_range &e) {
            std::cerr << "sysexd: " << "(invalid port)" << std::endl;
          }
          
          // Construct response
          messageOut = constructMessage("send", success ? "true" : "false");
        }
        else if (type == "send") {
          /**
           * send
           *
           * Sends a message on a given MIDI interface port.
           *
           * @param Object send parameters.
           * {
           *   port: (String) port index as available from query's ports Object.
           *   message: (String) base64 string containing message data.
           *   resend: (String) true or false, whether to wait for a message and resend if necessary.
           * }
           *
           * @returns Object
           * {
           *   "status": (Boolean) success or failure status.
           * }
           */
          // Indicate failure by default
          bool success = false;
          try
          {
            // Parse send parameters
            unsigned port = std::stoul(rootIn.get<std::string>("port"));
            std::string message = rootIn.get<std::string>("data");
            bool resend = rootIn.get<std::string>("resend") == "true" ? true : false;
            
            // Ensure the given port has been opened
            info->getSysexInterface().ensurePortOpened(port);
            
            // Decode base64
            message = websocketpp::base64_decode(message);
          
            // Send message
            if (m_debug) {
              std::cout << "sysexd: " << "sending sysex message of length " << message.size() << std::endl;
            }
            try {
              info->getSysexInterface().sendMessage(message, resend);
            }
            catch (const std::exception &e) {
              std::cerr << "sysexd: " << "(midi send exception) " << e.what() << std::endl;
            }
          
            // Indicate success
            success = true;
          }
          catch (const boost::property_tree::json_parser::json_parser_error &e) {
            std::cerr << "sysexd: " << "(json parse exception) " << e.message() << std::endl;
          }
          catch (const std::invalid_argument &e) {
            std::cerr << "sysexd: " << "(invalid port)" << std::endl;
          }
          catch (const std::out_of_range &e) {
            std::cerr << "sysexd: " << "(invalid port)" << std::endl;
          }
          
          // Construct response
          messageOut = constructMessage("send", success ? "true" : "false");
        }
        
        // Send output message
        sendMessage(hdl, messageOut);
      }
      else {
        // Invalid token
        if (m_debug) {
          std::cerr << "sysexd: " << "(invalid token)" << std::endl;
        }
      }
    }
    catch (const boost::property_tree::ptree_bad_path &e) {
      if (m_debug) {
        std::cerr << "sysexd: " << "(invalid message) " << e.what() << std::endl;
      }
    }
    catch (const boost::property_tree::json_parser::json_parser_error &e) {
      std::cerr << "sysexd: " << "(json parse exception) " << e.what() << std::endl;
    }
    catch (const std::out_of_range) {
      std::cerr << "sysexd: " << "(invalid connection)" << std::endl;
    }
    catch (const std::exception &e) {
      std::cerr << "sysexd: " << "(unhandled exception) " << e.what() << std::endl;
    }
  }
  
private:
  // MIDI SysEx interface class
  class SysexInterface {
  public:
    const static unsigned int INVALID_PORT = -1;
    
    const static unsigned int RETRIES = 10;
    const static unsigned int SLEEP = 150000;
    
    typedef std::function<void(const std::string)> errorCallback_t;
    typedef std::function<void(const std::string)> messageCallback_t;

    SysexInterface(bool debug = false)
    : m_midiIn(nullptr)
    , m_midiOut(nullptr)
    , m_portOpened(INVALID_PORT)
    , m_debug(debug)
    , state(0)
    {
      midiOpen();
    }
    
    ~SysexInterface() {
      midiClose();
    }
    
    void setDebug(const bool debug) {
      m_debug = debug;
    }
    
    void setCallbacks(const errorCallback_t fnErrorIn, const errorCallback_t fnErrorOut, const messageCallback_t fnMessage) {
      m_fnErrorIn = fnErrorIn;
      m_fnErrorOut = fnErrorOut;
      m_fnMessage = fnMessage;
    }

    void sendMessage(const std::string &message, bool resend) {
      std::vector<unsigned char> messageVector(message.begin(), message.end()); 
      
      // Set state
      state = 1;
      
      // Send message
      m_midiOut->sendMessage(&messageVector);
      
      // Resend functionality
      if (resend) {
        unsigned int retries = 0;
        while(retries < RETRIES) {
          usleep(SLEEP);
          if (state == 0) {
            // Message received
            return;
          }
          if (m_debug) {
            std::cout << "sysexd: " << "(resending message)" << std::endl;
          }
          m_midiOut->sendMessage(&messageVector);
          ++retries;
        }
      }
    }

    boost::property_tree::ptree getPorts() {
      boost::property_tree::ptree ports, inports, outports;
      {
        const unsigned int numPorts = m_midiIn->getPortCount();
        for(unsigned int i = 0; i < numPorts; ++i) {
          boost::property_tree::ptree port;
          port.put("name", m_midiIn->getPortName(i));
          inports.push_back(std::make_pair("", port));
        }
      }
      {
        const unsigned int numPorts = m_midiOut->getPortCount();
        for(unsigned int i = 0; i < numPorts; ++i) {
          boost::property_tree::ptree port;
          port.put("name", m_midiOut->getPortName(i));
          outports.push_back(std::make_pair("", port));
        }
      }
      ports.add_child("inports", inports);
      ports.add_child("outports", outports);
      return ports;
    }
    
    bool openInputPort(unsigned int port) {
      // Ensure the input port is closed
      if (m_midiIn->isPortOpen()) {
        m_midiIn->closePort();
      }
      
      // Open the specified port
      try {
        m_midiIn->openPort(port);
        m_midiIn->setCallback(&SysexInterface::midiMessageHandler, (void *)this);
        m_midiIn->ignoreTypes(false, false, false);
        if (m_debug) {
          std::cout << "sysexd: " << "set input port to " << port << " (" << m_midiIn->getPortName(port) << ")" << std::endl;
        }
        return true;
      }
      catch (const std::exception &e) {
        std::cerr << "sysexd: " << "(midi port exception) " << e.what() << std::endl;
      }
      return false;
    }
    
    bool ensurePortOpened(unsigned int port) {
      // Check if the given port corresponds to the currently configured MIDI output port
      if (m_portOpened == INVALID_PORT || port != m_portOpened) {
        // Make sure the port is closed in the MIDI output interface, to force opening the new port below
        m_midiOut->closePort();
        m_portOpened = INVALID_PORT;
      }
      // Ensure the port is opened on our MIDI output interface
      if (!m_midiOut->isPortOpen()) {
        try {
          m_midiOut->openPort(port);
          if (m_debug) {
            std::cout << "sysexd: " << "set output port to " << port << " (" << m_midiOut->getPortName(port) << ")" << std::endl;
          }
          m_portOpened = port;
          return true;
        }
        catch (const std::exception &e) {
          std::cerr << "sysexd: " << "(midi port exception) " << e.what() << std::endl;
        }
      }
      return false;
    }
    
    // RtMidiIn error handler
    static void midiInErrorHandler(RtMidiError::Type type, const std::string &message, void *thisPtr) {
      static_cast<SysexInterface *>(thisPtr)->m_fnErrorIn(message);
    }

    // RtMidiOut error handler
    static void midiOutErrorHandler(RtMidiError::Type type, const std::string &message, void *thisPtr) {
      static_cast<SysexInterface *>(thisPtr)->m_fnErrorOut(message);
    }
  
    // RtMidi message handler
    static void midiMessageHandler(double timestamp, std::vector<unsigned char> *message, void *thisPtr) {
      // Ignore everything but SysEx
      if (message->at(0) == 0xF0) {
        // Reset state
        static_cast<SysexInterface *>(thisPtr)->state = 0;
        
        // Convert message to base64 string
        std::string messageEncoded = websocketpp::base64_encode(message->data(), message->size());
        static_cast<SysexInterface *>(thisPtr)->m_fnMessage(messageEncoded);
      }
    }

  private:
    bool midiOpen() {
      if (m_debug) {
        std::cout << "sysexd: " << "opening MIDI interface" << std::endl;
      }
    
      // Ensure no MIDI interfaces exist
      midiClose();

      // Create MIDI interfaces
      try {
        m_midiIn = new RtMidiIn();
        m_midiOut = new RtMidiOut();
      } catch(RtMidiError &e) {
        std::cerr << "Could not create MIDI interface: " << e.what() << std::endl;
        return false;
      }
  
      // Set callbacks
      m_midiIn->setErrorCallback(&SysexInterface::midiInErrorHandler, (void *)this);
      m_midiOut->setErrorCallback(&SysexInterface::midiOutErrorHandler, (void *)this);
    
      return true;
    }
  
    void midiClose() {
      if (m_midiIn && m_midiOut) {
        if (m_debug) {
          std::cout << "sysexd: " << "closing MIDI interface" << std::endl;
        }
    
        // Delete MIDI interfaces
        delete m_midiOut;
        delete m_midiIn;
      }
    }

  public:        
    // Callbacks (must be accessible from static callbacks)
    errorCallback_t m_fnErrorIn;
    errorCallback_t m_fnErrorOut;
    messageCallback_t m_fnMessage;

    // Send state
    volatile int state;

  private:
    // RtMidi interfaces
    RtMidiIn *m_midiIn;
    RtMidiOut *m_midiOut;
    
    // Currently opened output port
    unsigned int m_portOpened;

    // Debug verbosity
    bool m_debug;
  };
  
  // Connection info class
  class ConnectionInfo {
  public:
    ConnectionInfo()
    {}

    SysexInterface& getSysexInterface() {
      return m_sysexInterface;
    }
    
  private:
    // SysEx interface
    SysexInterface m_sysexInterface;
  };
  
  // Typedefs
  typedef std::set<connection_hdl, std::owner_less<connection_hdl>> connectionSet_t;
  typedef std::map<connection_hdl, std::shared_ptr<ConnectionInfo>, std::owner_less<connection_hdl>> connectionInfoMap_t;
  
  // Server endpoint variables
  server m_server;
  connectionSet_t m_connections;
  connectionInfoMap_t m_connectionInfos;
  
  // Debug verbosity
  bool m_debug;
  
  // Authentication token
  std::string m_token;
};

int main() {
  const unsigned int listenPort = 9002;
  SysexServer server("5UOfQAtnjnNIaZUWzpX2LLBkHNxrXALECEpj0ssklTM7ptYCuSOVQNn0qemO8Zat", true);
  
  server.run(listenPort);
  
  return 0;
}
