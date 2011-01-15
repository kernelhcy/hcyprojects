package problem5;

import java.util.ArrayList;
import java.util.Date;

public class IncomeAndPayoutInfo
{
	public IncomeAndPayoutInfo()
	{
		incomes = new ArrayList<MoneyItem>();
		payout = new ArrayList<MoneyItem>();
	}
	
	public void addIncome(MoneyItem i)
	{
		incomes.add(i);
	}
	
	public void deleteIncome(MoneyItem i)
	{
		incomes.remove(i);
	}
	
	public void deleteIncome(int idx)
	{
		incomes.remove(idx);
	}
	
	public void addPayout(MoneyItem i)
	{
		payout.add(i);
	}
	
	public void deletePayout(MoneyItem i){
		payout.remove(i);
	}
	
	public void deletePayout(int idx){
		payout.remove(idx);
	}
	
	public MoneyItem getIncome(int idx)
	{
		return incomes.get(idx);
	}
	public MoneyItem getPayout(int idx)
	{
		return payout.get(idx);
	}
	
	public int getIncomesCnt()
	{
		return incomes.size();
	}
	
	public int getPayoutCnt()
	{
		return payout.size();
	}
	
	public double staticticsPayout(Date from, Date to)
	{
		double sum = 0;
		for(MoneyItem i : payout){
			if(i.getTime().after(from)
					&& i.getTime().before(to)){
				sum += i.getMoney();
			}
		}
		return 0;
	}
	
	public double staticticsIncome(Date from, Date to)
	{
		double sum = 0;
		for(MoneyItem i : incomes){
			if(i.getTime().after(from)
					&& i.getTime().before(to)){
				sum += i.getMoney();
			}
		}
		return 0;
	}
	
	private ArrayList<MoneyItem> incomes;
	private ArrayList<MoneyItem> payout;
}
