
Facebook
/
jefe
Público
Facebook Open Switching System Software para controlar conmutadores de red.

Licencia
 Ver licencia
 798 estrellas 273 tenedores 
Código
Cuestiones
dieciséis
Solicitudes de extracción
15
Comportamiento
Proyectos
Seguridad
Perspectivas
facebook/fboss
Última confirmación
@facebook-github-bot
Bot de código abierto y facebook-github-bot
…
hace 2 minutos
Estadísticas de Git
archivos
LÉAME.md
Sistema de cambio abierto de Facebook (FBOSS)
Apoya a Ucrania

FBOSS es la pila de software de Facebook para controlar y administrar conmutadores de red.

Componentes
FBOSS consta de una serie de aplicaciones, bibliotecas y utilidades de espacio de usuario.

El lanzamiento inicial de código abierto de FBOSS consiste principalmente en el daemon del agente, pero también estamos trabajando en el lanzamiento de piezas y funcionalidades adicionales.

agente daemon
Una de las piezas centrales de FBOSS es el demonio agente, que se ejecuta en cada conmutador y controla el ASIC de reenvío de hardware. El demonio del agente envía información de reenvío al hardware e implementa algunos protocolos del plano de control, como ARP y NDP. El agente proporciona API de ahorro para administrar rutas, para permitir que los procesos de control de enrutamiento externo programen su información de enrutamiento en las tablas de reenvío de hardware.

El código del agente se puede encontrar en fboss/agent

El agente requiere un archivo de configuración JSON para especificar su puerto y configuración de VLAN. Algunos archivos de configuración de muestra se pueden encontrar en fboss/agent/configs. Estos archivos no están realmente destinados al consumo humano; en Facebook tenemos herramientas que generan estos archivos para nosotros.

Demonio de enrutamiento
El agente FBOSS administra las tablas de reenvío en el ASIC de hardware, pero debe estar informado de las rutas actuales a través de las API de ahorro.

Nuestra versión inicial de código abierto aún no contiene un demonio de protocolo de enrutamiento capaz de comunicarse con el agente. El demonio del protocolo de enrutamiento que usamos en Facebook es bastante específico para nuestro entorno y probablemente no será tan útil para la comunidad de código abierto. Para un uso más general fuera de Facebook, debería ser posible modificar las herramientas de enrutamiento de código abierto existentes para hablar con el agente FBOSS, pero aún no lo hemos implementado. Mientras tanto, hemos incluido una pequeña secuencia de comandos de Python de muestra en fboss/agent/tools que puede agregar y eliminar rutas manualmente.

Herramientas administrativas
Obviamente, se requieren herramientas y utilidades adicionales para interactuar con el agente FBOSS, informar su estado, generar archivos de configuración y depurar problemas.

Por el momento, no tenemos muchas de nuestras herramientas listas para el lanzamiento de código abierto, pero esperamos tener más disponibles en las próximas semanas. Mientras tanto, el compilador de ahorro puede generar automáticamente un script python-remote para permitir la invocación manual de las diversas interfaces de ahorro del agente.

Edificio
Consulte el documento BUILD.md para obtener instrucciones sobre cómo compilar FBOSS.

Desarrollo futuro
FBOSS ha sido diseñado específicamente para manejar las necesidades de las redes del centro de datos de Facebook, pero esperamos que también pueda ser útil para la comunidad en general. Sin embargo, tenga en cuenta que esta versión inicial de FBOSS probablemente requerirá modificaciones y desarrollo adicional para admitir otras configuraciones de red más allá de las funciones utilizadas por Facebook. Hasta que madure más, FBOSS probablemente será de interés principalmente para los desarrolladores de software de red, en lugar de para los administradores de red que esperan usarlo como una solución llave en mano.

Esperamos recibir comentarios de la comunidad y esperamos que FBOSS pueda servir como punto de partida para otros usuarios que deseen programar conmutadores de red.

El desarrollo de FBOSS está en curso en Facebook y planeamos continuar lanzando más componentes, características adicionales y mejoras a las herramientas existentes.

Licencia
Ver LICENCIA .
