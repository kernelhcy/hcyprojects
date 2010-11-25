package problem8;

/**
 * 图书的编号、书名、类型、作者、价格、出版目期、出版社、购入日期、借阅人
 * @author hcy
 *
 */
public class Book
{
	
	public Book() {
		// TODO Auto-generated constructor stub
		this("", "", "", "", 0.0, "2010/10/1"
				, "", "2010/10/1", "");
	}
	
	public Book(String iSBN, String name, String style, String author,
			double price, String publishTime, String publisher,
			String buyTime, String borrower) {
		super();
		ISBN = iSBN;
		this.name = name;
		this.style = style;
		this.author = author;
		this.price = price;
		this.publishTime = publishTime;
		this.publisher = publisher;
		this.buyTime = buyTime;
		this.borrower = borrower;
	}
	
	public String getISBN()
	{
		return ISBN;
	}
	public void setISBN(String iSBN)
	{
		ISBN = iSBN;
	}
	public String getName()
	{
		return name;
	}
	public void setName(String name)
	{
		this.name = name;
	}
	public String getStyle()
	{
		return style;
	}
	public void setStyle(String style)
	{
		this.style = style;
	}
	public String getAuthor()
	{
		return author;
	}
	public void setAuthor(String author)
	{
		this.author = author;
	}
	public double getPrice()
	{
		return price;
	}
	public void setPrice(double price)
	{
		this.price = price;
	}
	public String getPublishTime()
	{
		return publishTime;
	}
	public void setPublishTime(String publishTime)
	{
		this.publishTime = publishTime;
	}
	public String getPublisher()
	{
		return publisher;
	}
	public void setPublisher(String publisher)
	{
		this.publisher = publisher;
	}
	public String getBuyTime()
	{
		return buyTime;
	}
	public void setBuyTime(String buyTime)
	{
		this.buyTime = buyTime;
	}
	public String getBorrower()
	{
		return borrower;
	}
	public void setBorrower(String borrower)
	{
		this.borrower = borrower;
	}
	
	public String toString()
	{
		StringBuffer sb = new StringBuffer("BOOK: [");
		sb.append("ISBN=");sb.append(ISBN);sb.append(',');
		sb.append("Name=");sb.append(name);sb.append(',');
		sb.append("Style=");sb.append(style);sb.append(',');
		sb.append("Author=");sb.append(author);sb.append(',');
		sb.append("Pirce=$");sb.append(price);sb.append(',');
		sb.append("Publish Time=");sb.append(publishTime.toString());
		sb.append(',');
		sb.append("Publisher=");sb.append(publisher);sb.append(',');
		sb.append("Buy Time=");sb.append(buyTime);sb.append(',');
		sb.append("Borrower=");sb.append(borrower);
		sb.append(']');
		return sb.toString();
	}
	
	private String ISBN; 		//编号
	private String name;		//书名
	private String style;		//类型
	private String author;		//作者
	private double price; 		//价格
	private String publishTime; 	//出版时间
	private String publisher; 	//出版社
	private String buyTime;		//购入时间
	private String borrower;	//借阅人
}
