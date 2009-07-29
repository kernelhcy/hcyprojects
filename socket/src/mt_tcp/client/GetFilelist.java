/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package mt_tcp.client;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.Socket;
import java.net.UnknownHostException;
import javax.swing.JOptionPane;
import mt_tcp.server.MyLogger;

/**
 *
 * @author hcy
 */
public class GetFilelist implements Runnable
{

    public GetFilelist(String host, int port, String dir)
    {
        if (null == dir)
        {
            this.dir = "/";
        } else
        {
            this.dir = dir;
        }
        this.hostName = host;
        this.port = port;

        logger = MyLogger.getInstance();

    }

    /**
     *
     */
    @Override
    public void run()
    {
        int state = createConnecion();
        if (state == NOSUCHDIRECTORY)
        {
            logger.info("No such directory");
        } else if (state == CONNECIONFAILED)
        {
            logger.info("Connecion failed");
        } else if (state == UNKOWNHOST)
        {
            logger.info("Unkown host");
        } else if (state == BADPORTNUMBER)
        {
            logger.info("Bad port number");
        }

        state = getFilelist();
    }

    /**
     * 建立socket连接
     * @return
     */
    int createConnecion()
    {
        try
        {
            //create socket connection
            connection = new Socket(this.hostName, this.port);

            logger.info("Create conneciont success");

            //get input and output stream
            fromServer = new BufferedReader(new InputStreamReader(connection.getInputStream()));
            toServer = new BufferedOutputStream(connection.getOutputStream());

        } catch (UnknownHostException ex)
        {
            logger.info(ex.getMessage());
            return UNKOWNHOST;
        } catch (IOException ex)
        {
            logger.info(ex.getMessage());
            return CONNECIONFAILED;
        }

        return SUCCESS;
    }

    /**
     *
     * @throws java.io.IOException
     */
    private int getFilelist()
    {
        try
        {
            logger.info("I want file list");
            //tell the server that I want the file list
            toServer.write("Filelist".getBytes());
            toServer.flush();
            //get the ack.
            //make sure that the server has received the required and is going to send back the file list
            //fromServer.readLine();

            logger.info("Dir:" + dir);
            //send the directory
            logger.info("Send dir to server.");
            toServer.write(this.dir.getBytes());
            toServer.flush();

            logger.info("Read file list from server.");
            //receive the file list
            String input = fromServer.readLine();
            //logger.info(input);
            logger.info("Read over.");


            StringBuilder filelists = new StringBuilder();

            if ( input != null)
            {
                names = input.split(",");
                for (String s : names)
                {
                    //System.out.println(s);
                    if (s.charAt(0) != '.')
                    {
                        //System.out.println(s);
                        filelists.append(s);
                        filelists.append('\n');
                    }
                }
            }
            else
            {
                filelists.append("No Files!");
            }

            JOptionPane.showMessageDialog(null, filelists.toString(), "FileLists",
                    JOptionPane.WARNING_MESSAGE, null);

        } catch (IOException ex)
        {
            logger.info(ex.getMessage());
            return NOSUCHDIRECTORY;
        } finally
        {
            try
            {
                connection.close();
                fromServer.close();
                toServer.close();
                logger.info("GetFilelist: Close connection.");
            } catch (IOException ex)
            {
            }
        }

        return SUCCESS;
    }
    /**
     * *********************************************
     *  成员变量
     * *********************************************
     */
    private String hostName = null;
    private int port = 0;
    private String dir = null;
    private Socket connection = null;
    private BufferedReader fromServer = null;
    private BufferedOutputStream toServer = null;
    private String[] names = null;
    public static final int SUCCESS = 0;
    public static final int UNKOWNHOST = -1;
    public static final int NOSUCHDIRECTORY = -2;
    public static final int BADPORTNUMBER = -3;
    public static final int CONNECIONFAILED = -4;
    private MyLogger logger = null;
}
