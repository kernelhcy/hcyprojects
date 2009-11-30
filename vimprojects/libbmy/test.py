import libbmy
hcy=libbmy.client()
hcy.login("hcy", "hcy1988hcy")
article={"title":"test", "board":"water", "text":"test"}
#hcy.post(article)
lists=hcy.get_board_list();
print(lists)
hcy.logout();

