/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package mt_tcp.server;

/**
 *
 * @author hcy
 */
public class CLIServer
{

    public static void main(String[] args)
    {
        MainAcceptThread mat =new MainAcceptThread(1234);
        mat.acceptConnection();
        
    }
}
