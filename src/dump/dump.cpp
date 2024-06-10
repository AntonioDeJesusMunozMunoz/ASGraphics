/*SEMAFOROS
	estan lo semáforos binario y los de timeline, solo usaremos los binarios

	al iniciar una operación, le damos un semáforo:
	-si lo ponemos como signal, se pondrá en verde cuando termina la operación
	-si lo ponemos como wait, la operación no iniciará hasta que el semáforo esté en verde

	En cuando el que lo tenga como wait inicie, el semáforo se resetea y está listo para otro uso
	*/
	/*FENCES
	este se usa para sincronizar el cpu en vez de gpu

	al iniciar una operación le damos la fence:
	-al proceso para que la ponga en verde cuando termine
	-llamamos una función que hará al cpu esperar a que la fence esté en verde

	Estas deben ser reseteadas manualmente
	En general se prefieren semáforors pq detener al cpu no es ideal
	*/

	//Lo regreso a la swapChain

	//Esperar a que el anterior frame termine: fence
	//dibujar varios frames en vez de esperar a que se termine de dibujar el anterior //osea continuar dibujando frames para crear un backlog
	//si quiero dibujar más de uno necesito:
		//no pasarme de las imágenes de la swapchain
			//espero a que la cantidad de imágenes usadas sea menor a la cantidad de imágenes en la swapchain
			//luego pido otro frame
		//no resetear frameBuffers que se estén usando
			//tener la misma cantidad de buffers que imágenes?
			//keep track of frames drawn
			//el commandBuffer que reseteo sería commandBuffers[framesBeingDrawn - 1]
		//que cada writeToBuffer espere a que su imagen haya sido conseguida
			//semáforo para cada imagen?
			//lo mismo que command buffer?


	//si no esperara:
		//obtendría la siguiente imagen
			//eventualmente se me acabarían las imágenes de la swapChain*
		//resetearía el commandBuffer
			//el commandBuffer se resetearía mientras se está usando*
		//esperaría a que el semáforo me de greenligth para conseguir imagen
			//todos los frames avanzarían en cuanto uno de ellos diera greenligth
		//iría de que wowowowow lopeando por drawFrame cuando no pudiera ejecutar

