package problem5;

import java.util.Calendar;
import java.util.Date;

public class MoneyItem
{
	
	public MoneyItem(String name, String fromOrGoWhere, Date time
			, double money) {
		super();
		this.name = name;
		this.fromOrGoWhere = fromOrGoWhere;
		this.time = time;
		this.money = money;
		this.id = MoneyItem.ID;
		++MoneyItem.ID;
	}
	
	public MoneyItem() {
		// TODO Auto-generated constructor stub
		this("", "", Calendar.getInstance().getTime(), 0.0);
	}
	
	
	@Override
	public String toString()
	{
		return "MoneyItem [id=" + id + ", name=" + name
				+ ", fromOrGoWhere=" + fromOrGoWhere
				+ ", time=" + time + ", money=" 
				+ money + "]";
	}

	public long getId()
	{
		return id;
	}

	public String getName()
	{
		return name;
	}
	public void setName(String name)
	{
		this.name = name;
	}

	public String getFromOrGoWhere()
	{
		return fromOrGoWhere;
	}

	public void setFromOrGoWhere(String fromOrGoWhere)
	{
		this.fromOrGoWhere = fromOrGoWhere;
	}

	public Date getTime()
	{
		return time;
	}
	public void setTime(Date time)
	{
		this.time = time;
	}
	public double getMoney()
	{
		return money;
	}
	public void setMoney(double money)
	{
		this.money = money;
	}
	
	private long id;
	private String name;
	private String fromOrGoWhere;
	private Date time;
	private double money;
	
	private static int ID = 0;
}
