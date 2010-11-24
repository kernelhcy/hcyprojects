package problem8.eventagent;

/**
 * The event class
 * 
 * Describe the name of the event and the arguments.
 * @author hcy
 *
 */
public class Event
{
	public Event(String name)
	{
		this.name = name;
		/**
		 * I think that 16 arguments are enough to 
		 * any event.
		 * If you really have more arguments, you can
		 * encapsulate these arguments into a class.
		 */
		args = new Object[16];
		nArgs = 0;
	}
	
	/**
	 * Three most usual version. 
	 */
	public Event(String name, Object arg)
	{
		this(name);
		args[0] = arg;
		nArgs = 1;
	}
	
	public Event(String name, Object arg1, Object arg2)
	{
		this(name);
		args[0] = arg1;
		args[1] = arg2;
		nArgs = 2;
	}
	
	public Event(String name, Object arg1, Object arg2, Object arg3)
	{
		this(name);
		args[0] = arg1;
		args[1] = arg2;
		args[2] = arg3;
		nArgs = 3;
	}
	
	public void clearArg()
	{
		nArgs = 0;
	}
	
	public void addArg(Object arg)
	{
		if(nArgs >= 16){
			System.out.println("Can not add more than 16 " +
					"arguments!");
			return;
		}
		args[nArgs] = arg;
		++nArgs;
	}
	
	public String getName()
	{
		return name;
	}
	
	public Object[] getArgs()
	{
		return args;
	}
	
	public int getArgsNum()
	{
		return nArgs;
	}
	
	private String name;
	private Object[] args;
	private int nArgs;
}
