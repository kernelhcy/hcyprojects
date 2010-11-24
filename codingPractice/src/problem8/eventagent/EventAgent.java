package problem8.eventagent;

import java.util.HashMap;
import java.util.Map;

/**
 * This is an event agent.(or event dispatcher, or some thing else,
 * It is just what it is...)
 * 
 * I think this is not thread safe.
 * 
 * @author hcy
 *
 */
public class EventAgent
{
	static{
		map = new HashMap<String, EventResponser>();
	}
	
	/**
	 * Register an event responser.
	 * @param er
	 */
	public static void registerEvent(EventResponser er)
	{
		if(map == null){
			return;
		}
		if(map.containsKey(er.getName())){
			return;
		}
		map.put(er.getName(), er);
	}
	
	/**
	 * Unregister an event response.
	 * @param er
	 */
	public static void unregisterEvent(EventResponser er)
	{
		if(er == null){
			return;
		}
		
		map.remove(er.getName());
	}
	
	/**
	 * Dispatch an event.
	 * @param e
	 */
	public static void dispatchEvent(Event e)
	{
		if(e == null){
			return;
		}
		if(!map.containsKey(e.getName())){
			System.err.println("Unknown event:" + e.getName());
			return;
		}
		EventResponser er = map.get(e.getName());
		/*
		 * Maybe the result of the action should return
		 * to the event creator.
		 * But I do not use the result just now, so
		 * throw the result away.
		 */
		er.doAction(e.getArgs(), e.getArgsNum());
	}
	
	public static void printAllEvent()
	{
		System.out.println("All registered events:");
		for(String name : map.keySet()){
			System.out.println("\t" + name);
		}
	}
	
	private static Map<String, EventResponser> map;
}
