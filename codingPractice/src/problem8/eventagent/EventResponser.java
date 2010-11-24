package problem8.eventagent;

/**
 * This is the interface of an event responser.
 * 
 * If one class want to register some events, it
 * MUST be create an event class or some other class
 * that implements this interface.
 * 
 * @author hcy
 *
 */
public interface EventResponser
{
	/**
	 * The event agent will call this function to response
	 * to this event.
	 * 
	 * @param args : the array of the arguments.
	 * 	NOTE: the order of the arguments in the args is
	 * 		the same of event responsing function.
	 * @param argc : the number of arguments in args
	 */
	public void doAction(Object[] args, int argc);
	
	/**
	 * The name of the event.
	 * @return
	 */
	public String getName();
}
