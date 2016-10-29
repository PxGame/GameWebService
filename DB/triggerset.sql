drop trigger if exists insertloginlog;
drop trigger if exists logincntincrement;
delimiter $$
create trigger logincntincrement before update on user for each row
begin
	declare _cnt int default 0;
    set _cnt = new.logincount + 1;
	set new.logincount = _cnt;
end$$

create trigger insertloginlog after update on user for each row
begin
	insert into loginlog(name,ip,time) value(new.name, new.lastloginip, new.lastlogintime);
end$$