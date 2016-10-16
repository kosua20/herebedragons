//
//  GameViewController.swift
//  DragonSceneKit
//
//  Created by Simon Rodriguez on 11/10/2016.
//  Copyright (c) 2016 Simon Rodriguez. All rights reserved.
//

import SceneKit
import QuartzCore

class GameViewController: NSViewController {
    
	@IBOutlet var gameView: SCNView!

    
    override func awakeFromNib(){
        super.awakeFromNib()
        
		// create a new scene
		let scene = SCNScene(named: "art.scnassets/DragonScene.scn")!
		
		// set the scene to the view
		self.gameView!.scene = scene
		
		// allows the user to manipulate the camera
		self.gameView!.allowsCameraControl = true
		
		// show statistics such as fps and timing information
		self.gameView!.showsStatistics = true
		
		// configure the view
		self.gameView!.backgroundColor = NSColor.black
	
		
    }

}
