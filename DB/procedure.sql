delimiter $$

drop procedure if exists user_login_update $$ 

create procedure user_login_update(in n varchar(255), in i varchar(255), in t varchar(255))
begin
    declare _cnt int default 0;
    declare _time datetime default now();
	declare _error int default false;
	declare continue handler for sqlexception set _error = true;#非声明语句必须放在所有非声明语句的后面，否者会报错。
    
    start transaction;
    
		select logincount into _cnt from user where name = n;
		set _cnt = _cnt + 1;

		update user set lastlogintime = _time,lastloginip = i,logincount = _cnt where name = n;
		if t != '' then
			update user set logintoken = t where name = n;
		end if;

		insert into loginlog(name,ip,time) value(n,i,_time);
    
    if _error = false then
		commit;
	else
		rollback;
		select _error;
	end if;
    
end$$

#====================================================================

drop procedure if exists user_query $$ 

create procedure user_query(in n varchar(255))
BEGIN
	declare _error bool default false;
	declare continue handler for sqlexception set _error = true;#非声明语句必须放在所有非声明语句的后面，否者会报错。
    
    start transaction;
    
		select * from user where name = n;
    
    if _error = false then
		commit;
	else
		rollback;
        select _error;
	end if;
END$$

#====================================================================

drop procedure if exists user_regist $$ 

create procedure user_regist(in n varchar(255), in p varchar(255))
BEGIN
	declare _ret bool default false;
	declare _cnt int default 0;
	declare _error bool default false;
	declare continue handler for sqlexception set _error = true;#非声明语句必须放在所有非声明语句的后面，否者会报错。
    
    start transaction;
    
		select count(*) into _cnt from user where user.name = n;
		if (_cnt = 0) then
			set _ret = true;
			insert into user(name,pwd,createtime) value(n,p,now());
		else
			set _ret = false;
		end if;
		select _ret;
    
	if _error = false then
		commit;
	else
		rollback;
        select _error;
	end if;
END$$

delimiter ;