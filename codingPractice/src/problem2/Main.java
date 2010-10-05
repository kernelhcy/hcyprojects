package problem2;

public class Main
{

	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		// TODO Auto-generated method stub
		MyStack<Integer> ms = new MyStack<Integer>(10);
		
		ms.push(100);
		ms.push(200);
		ms.push(300);
		ms.push(400);
		ms.push(500);
		System.out.println("Stack size:" + ms.size());
		
		System.out.println("pop:" + ms.pop());
		System.out.println("pop:" + ms.pop());
		System.out.println("pop:" + ms.pop());
		System.out.println("pop:" + ms.pop());
		System.out.println("pop:" + ms.pop());
		
		System.out.println("Is empty?" + ms.empty());
		
		for(int i = 0; i < 10; ++i){
			ms.push(i);
		}
		System.out.println("Is full?" + ms.full());
		ms.print();
	}

}
