package problem5;

import java.util.ArrayList;

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
	
	public void deleteIncome(int id)
	{
		for(MoneyItem i : incomes){
			if(i.getId() == id){
				incomes.remove(i);
				break;
			}
		}
	}
	
	public void addPayout(MoneyItem i)
	{
		payout.add(i);
	}
	
	public void deletePayout(MoneyItem i){
		payout.remove(i);
	}
	
	public void deletePayout(int id){
		for(MoneyItem i : payout){
			if(i.getId() == id){
				payout.remove(i);
				break;
			}
		}
	}
	
	private ArrayList<MoneyItem> incomes;
	private ArrayList<MoneyItem> payout;
}
