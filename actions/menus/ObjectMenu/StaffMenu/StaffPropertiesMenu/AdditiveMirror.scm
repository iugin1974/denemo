;;;AdditiveMirror
(let ((params AdditiveMirror::params))
(if params
	(d-SubstituteMusic params)
	(d-SubstituteMusic (cons 'mix #f))))
