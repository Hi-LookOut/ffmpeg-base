package com.example.javaclass;

import java.util.concurrent.Exchanger;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

class Account {
    private String Name;
    private double asset = 0;
    private double charge = 0;
    private ReentrantLock slock = new ReentrantLock();
    private Condition putCondition = slock.newCondition();
    private Condition getCondition = slock.newCondition();

    public Account(String name, double asset, double charge) {
        Name = name;
        this.asset = asset;
        this.charge = charge;
    }

    public void save(double money) {
        this.slock.lock();
        try {
            if (asset >= 1000) {
                System.out.println("【系统提示】：滚犊子，有钱了不起啊");
                getCondition.await();
            }
            System.out.println("【加钱锁】");
            this.asset += money;
            TimeUnit.SECONDS.sleep(2);
            System.out.println("【加钱成功，卡上余额】：" + this.asset);
            putCondition.signal();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            this.slock.unlock();
            System.out.println("【解------加钱锁】");
        }
    }


    public void spend() {
        this.slock.lock();
        try {
            if (this.asset == 0 || this.asset < this.charge) {
                System.out.println("【系统提示】：卡上余额不足，请及时充值！！！");
                putCondition.await();
            }
            System.out.println("【消费锁】");
            TimeUnit.SECONDS.sleep(1);
            this.asset = this.asset - charge;
            System.out.println("【消费】：" + charge + "，【余额】：" + this.asset);
            getCondition.signal();
        } catch (InterruptedException e) {
            return;
        } finally {
            this.slock.unlock();
            System.out.println("【解-------消费锁】");
        }
    }

}

//public class myClass {
//    public static void main(String[] args) throws InterruptedException {
//        Account ac=new Account("何大侠",0,150);
//        for (int i=0;i<20;i++){
//            if (i%2==0){
//                new Thread(()->{
//                    ac.save(100);
//                }).start();
//            }else {
//                new Thread(()->{
//                    ac.spend();
//                }).start();
//            }
//        }
//    }
//}


public class myClass {
    public static void main(String[] args) throws InterruptedException {
        Exchanger<String> ec = new Exchanger<>();
        new Thread(() -> {
            try {
                System.out.println("【商家开始买东西，等待客户】");
                TimeUnit.SECONDS.sleep(5);
                System.out.println("5秒后，【商家】买东西了");
                String info = ec.exchange("商品");
                System.out.println("【商家获得】：" + info);
            } catch (InterruptedException e) {
                e.printStackTrace();
            } finally {
                System.out.println("交易成功");
            }
        }, "商家").start();

        new Thread(() -> {
            try {
                System.out.println("【客户到店，买东西】");
                TimeUnit.SECONDS.sleep(2);
                System.out.println("2秒后，【客户】卖东西了");
                String info = ec.exchange("money");
                System.out.println("【客户获得】：" + info);
            } catch (InterruptedException e) {
                e.printStackTrace();
            } finally {
                System.out.println("交易成功");
            }
        }, "客户").start();
    }

}


