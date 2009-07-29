/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package mt_tcp.server;

/**
 * 用于计算当前已经传送的文件长度。
 * 更新进度条。
 * @author hcy
 */
public class TransferLengthInfo
{
    public TransferLengthInfo()
    {
        length = 0;
    }

    synchronized public void addValue(long value)
    {
        length += value;
    }

    public long getLength()
    {
        return length;
    }

    private long length = 0;
}
