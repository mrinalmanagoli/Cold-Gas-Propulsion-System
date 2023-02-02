/**
 * Copyright 2017, Digi International Inc.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES 
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR 
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
package com.digi.xbee.api.senddata;

import com.digi.xbee.api.exceptions.XBeeException;
import com.digi.xbee.api.models.XBee64BitAddress;
import com.digi.xbee.api.models.XBeeMessage;
import com.digi.xbee.api.DigiMeshDevice;
import com.digi.xbee.api.RemoteDigiMeshDevice;
import java.util.concurrent.TimeoutException;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Arrays;

/**
 *
 * @author Mrinal
 *         Communication module's code running on the ground station.
 *         This is responsible for establishing a communication channel with the
 *         control module as well as the satellite communications sub-module.
 *         Once this is done, any data received from the control module is
 *         transmitted to the satellite
 * 
 */
public class MainApp {

    // Common parameters

    private static final byte ACK = (byte) 0xFF; // End of transmission. For terminating connections

    private static final byte INITIALIZATION_REQUEST = (byte) 0xFC;

    private static final byte INITIALIZATION_REQUEST_ACK = (byte) 0xFD; // End of Frame, indicating end of frames

    private static final byte READY = (byte) 0xFE; // Ready indicator, used in connection initialization

    // Port number on which server will listen to for the controller module
    private static final int PORT = 5555;

    // Max. buffer size for receiving messages from controller module
    private static final int BUFFER_SIZE = 8;

    private static final String PORT_RF_LINK = "COM10";

    private static final byte[] DESTINATION_MAC_ADDRESS = { (byte) 0x00, (byte) 0x13, (byte) 0xA2, (byte) 0x00,
            (byte) 0x41, (byte) 0x5B, (byte) 0xAD, (byte) 0x65 };

    // Baud rate of your sender module
    private static final int BAUD_RATE = 115200;

    // -----------------------------------------------------------------------------------------------------------------------

    private static ServerSocket serverSocket = null;

    private static InputStream input = null;

    private static OutputStream output;

    private static Socket clientSocket = null;

    private static InetAddress clientIPAddress = null;

    private static DigiMeshDevice groundStationRFLink;

    private static RemoteDigiMeshDevice cubesatRFLink;

    // -----------------------------------------------------------------------------------------------------------------------
    /**
     * Application main method.
     * 
     * @param args Command line arguments.
     */
    public static void main(String[] args) {
        initialize_rf_links();
        startServer();
        System.out.println("Listening to control module connections on 5555");
        clientIPAddress = listen();

        System.out.println("Control module connected as " + clientIPAddress.toString());

        // boolean handshake_result = handshake();
        boolean handshake_result = true;

        if (handshake_result) {
            // send control module a ready signal
            sendMessageToClient(READY);

            // Handle any data inputted by control module
            handleComms();
        } else {
            System.out.println("Handshake wasn't successful, exiting ...");
            System.exit(1);
        }
    }

    private static void handleComms() {
        byte[] buffer = new byte[BUFFER_SIZE];
        int counter = 0;

        while (true) {
            try {
                if (input.available() != 0) {
                    byte msg = getMessageFromClient();
                    buffer[counter++] = msg;

                    if (counter == BUFFER_SIZE) {
                        if (cubesatRFLink != null) {
                            groundStationRFLink.sendData(cubesatRFLink, Arrays.copyOfRange(buffer, 0, counter));
                        }

                        for (int i = 0; i < BUFFER_SIZE; i++) {
                            // System.out.println(buffer[i]);
                        }

                        buffer = new byte[BUFFER_SIZE];
                        counter = 0;
                    }
                }
            } catch (IOException ioe) {
                ioe.printStackTrace();
            } catch (XBeeException xbee) {
                xbee.printStackTrace();
            } catch (Exception e) {
                e.printStackTrace();
            }

        }
    }

    /*
     * Purpose: To perform handshake with the cubesat rf link
     * Parameters: None
     * Return: True, if initialization was a success, False if not
     */
    private static boolean handshake() {
        boolean ret_val = false;

        try {
            XBeeMessage xbeeMessage = groundStationRFLink.readData(Integer.MAX_VALUE);
            if (xbeeMessage.getData()[0] == INITIALIZATION_REQUEST) {
                groundStationRFLink.sendData(cubesatRFLink, new byte[] { INITIALIZATION_REQUEST_ACK });
                xbeeMessage = groundStationRFLink.readData(Integer.MAX_VALUE);

                if (xbeeMessage.getData()[0] == ACK) {
                    ret_val = true;
                }
            }
        } catch (XBeeException xbe) {
            xbe.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }

        return ret_val;
    }

    /*
     * Purpose: To initialize ground station and cubesat RF links
     * Parameters: None
     * Return: True, if initialization was a success, False if not
     */
    private static boolean initialize_rf_links() {
        boolean ret_val = false;

        try {
            groundStationRFLink = new DigiMeshDevice(PORT_RF_LINK, BAUD_RATE);
            groundStationRFLink.open();
            XBee64BitAddress destinationAddress = new XBee64BitAddress(DESTINATION_MAC_ADDRESS);
            cubesatRFLink = new RemoteDigiMeshDevice(groundStationRFLink, destinationAddress);
            ret_val = true;
        } catch (XBeeException xbe) {
            xbe.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }

        return ret_val;
    }

    /*
     * Purpose: To start up the server
     * Parameters: None
     * Return: None
     */
    private static void startServer() {
        if (serverSocket == null) {
            try {
                serverSocket = new ServerSocket(PORT);
            } catch (IOException e) {
                e.printStackTrace();
                System.out.println("Cannot create ServerSocket, exiting program.");
                System.exit(0);
            }
        } else {
            stopServer();
            System.exit(1);
        }
    }

    /*
     * Purpose: To stop the server from running
     * Parameters: None
     * Return: None
     */
    private static void stopServer() {
        if (serverSocket != null) {
            try {
                serverSocket.close();
            } catch (IOException e) {
                e.printStackTrace();
                System.out.println("Cannot close ServerSocket, exiting program.");
                System.exit(0);
            }
        }
    }

    /*
     * Purpose: To listen to new connections. This ia a BLOCKING CALL
     * Parameters: None
     * Return: The InetAddress of the client that connects.
     */
    private static InetAddress listen() {
        try {
            clientSocket = serverSocket.accept();
            input = clientSocket.getInputStream();
            output = clientSocket.getOutputStream();
        } catch (IOException e) {
            e.printStackTrace();
            System.out.println("listen exception, exiting program.");
            System.exit(0);
        } finally {
            return clientSocket.getInetAddress();
        }
    }

    /*
     * Purpose: To get a single byte message from the connected client
     * Parameters: None
     * Return: The byte read from the client
     */
    private static byte getMessageFromClient() {
        byte msg = 0x00;
        try {
            msg = (byte) input.read();
        } catch (IOException e) {
            e.printStackTrace();
            System.out.println("cannot read from socket; exiting program.");
            System.exit(0);
        } finally {
            return msg;
        }
    }

    /*
     * Purpose: To send a single byte message to the connected client
     * Parameters: msg: The byte to be sent
     * Return: None
     */
    private static void sendMessageToClient(byte msg) {
        try {
            output.write(msg);
            output.flush();
        } catch (IOException e) {
            e.printStackTrace();
            System.out.println("cannot send to socket; exiting program.");
            System.exit(0);
        } finally {
        }
    }

    /*
     * Purpose: To disconnect from the client
     * Parameters: None
     * Return: None
     */
    private static void disconnectClient() {
        try {
            if (clientSocket != null) {
                clientSocket.close();
            }
            input = null;
            clientSocket = null;
            System.out.println(clientIPAddress + " disconnected!");
        } catch (IOException e) {
            e.printStackTrace();
            System.err.println("cannot close stream/client socket; exiting.");
            System.exit(0);
        }
    }
}