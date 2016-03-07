//
//  ViewController.swift
//  RotationUpdater
//
//  Created by Tim Lindeberg on 05/03/16.
//  Copyright © 2016 TimLindeberg. All rights reserved.
//

import UIKit
import CoreMotion
import Foundation


class ViewController: UIViewController {

    static let UpdateInterval = NSTimeInterval(5)
    static let IP: String = "192.168.0.13"
    static let Port: UInt32 = 56789
    
    static let Red: UIColor = UIColor(colorLiteralRed: 1, green: 0, blue: 0, alpha: 1)
    static let Green: UIColor = UIColor(colorLiteralRed: 0, green: 1, blue: 0, alpha: 1)
    
    let motionManager: CMMotionManager = CMMotionManager()
    let connection = Connection()
    
    @IBOutlet weak var connectedStatus: UILabel!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        NSNotificationCenter.defaultCenter().addObserver(self, selector : "rotated", name: UIDeviceOrientationDidChangeNotification, object: nil)
    
        connection.onConnected({
            self.connectedStatus.textColor = ViewController.Green
            self.connectedStatus.text = "Connected"
        })
        
        connection.onDisconnected({
            self.connectedStatus.textColor = ViewController.Red
            self.connectedStatus.text = "Not Connected"
            self.connection.connect(ViewController.IP, port: ViewController.Port)
        })
        self.connection.connect(ViewController.IP, port: ViewController.Port)
    }

    func rotated(){
        if(UIDeviceOrientationIsLandscape(UIDevice.currentDevice().orientation)){
            connection.send("L")
        }else{
            connection.send("P")
        }
    }
    
    class Connection: NSObject, NSStreamDelegate {
        
        private var out: NSOutputStream!
        private var inp: NSInputStream!
        
        private var onConnectedFunc: (() -> Void)?
        private var onDisconnectedFunc: (() -> Void)?
        
        private var isConnected = false
        
        func onConnected(f: () -> Void){
            onConnectedFunc = f
        }
        
        func onDisconnected(f: () -> Void){
            onDisconnectedFunc = f
        }

        func send(s: String){
            if(out == nil || !isConnected){
                return
            }
            
            var buffer = [UInt8]((s + "\0").utf8) // null terminate
            out?.write(&buffer, maxLength: buffer.count)
        }
        
        func connect(host: String, port: UInt32){
            if(isConnected){
                return
            }
            
            var readStream: Unmanaged<CFReadStream>?
            var writeStream: Unmanaged<CFWriteStream>?
            
            CFStreamCreatePairWithSocketToHost(nil, host, port, &readStream, &writeStream)
            
            inp = readStream!.takeRetainedValue()
            out = writeStream!.takeRetainedValue()
            
            out.delegate = self
            inp.delegate = self
            
            out.scheduleInRunLoop(NSRunLoop.currentRunLoop(), forMode: NSDefaultRunLoopMode)
            inp.scheduleInRunLoop(NSRunLoop.currentRunLoop(), forMode: NSDefaultRunLoopMode)
            
            out.open()
            inp.open()
        }
        
        func stream(stream: NSStream, handleEvent eventCode: NSStreamEvent){
            if(stream === inp){
                switch eventCode {
                case NSStreamEvent.HasBytesAvailable: disconnect()
                default:                              break
                }
            }else{
                switch eventCode {
                case NSStreamEvent.OpenCompleted: connect()
                case NSStreamEvent.ErrorOccurred: disconnect()
                default:                          break
                }
            }
        }
        
        private func connect() {
            if(isConnected || onConnectedFunc == nil){
                return
            }
            isConnected = true
            onConnectedFunc!()
        }
        
        private func disconnect(){
            inp.close()
            out.close()
            
            inp = nil
            out = nil
            
            isConnected = false
            
            if(onDisconnectedFunc != nil){
                onDisconnectedFunc!()
            }

        }
    }


}

