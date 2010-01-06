/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.model.processor;

import java.util.HashMap;
import java.util.Map;

public class ClassProcessorRegistry
{
	
	protected static Map idToProcessorMap = new HashMap();
	
	public static void register(ClassProcessor classProcessor)
	{
		idToProcessorMap.put(classProcessor.getProcessorID(), classProcessor);
	}
	
	public static ClassProcessor getClassProcessor(String id)
	{
		return (ClassProcessor) idToProcessorMap.get(id);
	}
	
	public static ClassProcessor[] getClassProcessors()
	{
		return (ClassProcessor[]) idToProcessorMap.values().toArray(new ClassProcessor[0]);
	}
	
}
