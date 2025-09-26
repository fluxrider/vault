package com.muquit.libsodiumjna;

import org.apache.commons.codec.DecoderException;
import org.apache.commons.codec.binary.Hex;

/**
 * A helper class to convert hex to binary and vice versa. It uses apache commons Hex class.
 * I get confused (dyslexic) with encode decode methods of org.apache.commons.codec.binary.Hex class and
 * always have to look it up!
 * @author muquit@muquit.com - Oct-09-2016
 */

public class SodiumUtils
{
    
    /**
     * Convert a Hex string to Binary. It uses org.apache.commons.codec.binary.Hex.decodeHex(char[])
     * 
     * @param hexString Hex string to convert
     * @return Binary bytes
     * <p>
     * @author muquit@muquit.com - Sep 9, 2017
     */
    public static byte[] hex2Binary(String hexString)
    {
        byte[] data = null;
        try
        {
            data = Hex.decodeHex(hexString.toCharArray());
        } catch (DecoderException e)
        {
            e.printStackTrace();
            return null;
        }
        return data;
    }
    
    /**
     * Convert binary data to Hex string. It uses org.apache.commons.codec.binary.Hex.encodeHexString(byte[])
     * 
     * @param data Binary data bytes to convert
     * @return Hex string
     * <p>
     * @author muquit@muquit.com - Sep 9, 2017
     */
    public static String binary2Hex(byte[] data)
    {
        String hexString = Hex.encodeHexString(data);
        return hexString;
    }
    

}
