/*
Navicat MySQL Data Transfer

Source Server         : Webserver
Source Server Version : 50716
Source Host           : 192.168.2.115:3306
Source Database       : game_db

Target Server Type    : MYSQL
Target Server Version : 50716
File Encoding         : 65001

Date: 2016-10-27 23:11:37
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for loginlog
-- ----------------------------
DROP TABLE IF EXISTS `loginlog`;
CREATE TABLE `loginlog` (
  `index` int(11) NOT NULL AUTO_INCREMENT COMMENT '自增长序号',
  `username` varchar(255) DEFAULT '' COMMENT '用户名',
  `ip` varchar(255) DEFAULT '' COMMENT 'ip地址',
  `time` datetime DEFAULT NULL COMMENT '登录时间',
  PRIMARY KEY (`index`)
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of loginlog
-- ----------------------------

-- ----------------------------
-- Table structure for user
-- ----------------------------
DROP TABLE IF EXISTS `user`;
CREATE TABLE `user` (
  `username` varchar(255) NOT NULL COMMENT '用户名',
  `userpwd` varchar(255) DEFAULT NULL COMMENT '用户密码',
  `createtime` datetime DEFAULT NULL COMMENT '创建时间',
  `lastlogintime` datetime DEFAULT NULL COMMENT '最后依次登录时间',
  `logintoken` varchar(255) DEFAULT '' COMMENT '登录令牌，由用户名和密码还有登录次数生成MD5而得',
  `logincount` varchar(255) DEFAULT '' COMMENT '登录次数',
  PRIMARY KEY (`username`),
  KEY `username` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of user
-- ----------------------------
