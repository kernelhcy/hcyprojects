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
import java.util.logging.Level;
import java.util.logging.Logger;

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
        }
        else
        {
            this.dir = dir;
        }
        this.hostName = host;
        this.port = port;


    }

    /**
     *
     */
    @Override
    public void run()
    {
        int state = createConnecion();
        if(state == NOSUCHDIRECTORY)
        {
            System.out.println("No such directory");
        }
        else if(state == CONNECIONFAILED)
        {
            System.out.println("Connecion failed");
        }
        else if(state == UNKOWNHOST)
        {
            System.out.println("Unkown host");
        }
        else if(state == BADPORTNUMBER)
        {
            System.out.println("Bad port number");
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
            
            System.out.println("Create conneciont success");

            //get input and output stream
            fromServer = new BufferedReader(new InputStreamReader(connection.getInputStream()));
            toServer = new BufferedOutputStream(connection.getOutputStream());

        }
        catch (UnknownHostException ex)
        {
            System.out.println(ex.getMessage());
            return UNKOWNHOST;
        }
        catch (IOException ex)
        {
            System.out.println(ex.getMessage());
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
            System.out.println("I want file list");
            //tell the server that I want the file list
            toServer.write("Filelist".getBytes());
            toServer.flush();
            //get the ack.
            //make sure that the server has received the required and is going to send back the file list
            //fromServer.readLine();
            
            System.out.println("Dir:"+dir);
            //send the directory
            toServer.write(this.dir.getBytes());
            toServer.flush();

            System.out.println();
            //receive the file list
            String input = fromServer.readLine();
            System.out.println(input);
            System.out.println();
            names = input.split(",");
            for (String s : names)
            {
                System.out.println(s);
            }
        }
        catch (IOException ex)
        {
            System.out.println(ex.getMessage());
            return NOSUCHDIRECTORY;
        }
        finally
        {
            try
            {
                connection.close();
                fromServer.close();
                toServer.close();
            }
            catch (IOException ex)
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
}
