//
//  GameViewController.swift
//  DragonSceneKit-iOS
//
//  Created by Simon Rodriguez on 15/10/2016.
//  Copyright © 2016 Simon Rodriguez. All rights reserved.
//

import UIKit
import QuartzCore
import SceneKit

class GameViewController: UIViewController {
	
	@IBOutlet var gameView: SCNView!
	
    override func viewDidLoad() {
        super.viewDidLoad()
        
		// create a new scene
		let scene = SCNScene(named: "art.scnassets/DragonScene.scn")!
		
		// set the scene to the view
		self.gameView!.scene = scene
		
		// allows the user to manipulate the camera
		self.gameView!.allowsCameraControl = true
		
		// show statistics such as fps and timing information
		self.gameView!.showsStatistics = true
		
		// configure the view
		self.gameView!.backgroundColor = UIColor.black
    }
	
    
    override var shouldAutorotate: Bool {
        return true
    }
    
    override var prefersStatusBarHidden: Bool {
        return true
    }
    
    override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
        if UIDevice.current.userInterfaceIdiom == .phone {
            return .allButUpsideDown
        } else {
            return .all
        }
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Release any cached data, images, etc that aren't in use.
    }

}
