/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/icecube800bc/Icecube800bcPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
  "ports": {
    "1": {
        "mapping": {
          "id": 1,
          "name": "eth1/1/1",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "2": {
        "mapping": {
          "id": 2,
          "name": "eth1/1/5",
          "controllingPort": 2,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core0",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "3": {
        "mapping": {
          "id": 3,
          "name": "eth1/5/1",
          "controllingPort": 3,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "4": {
        "mapping": {
          "id": 4,
          "name": "eth1/5/5",
          "controllingPort": 4,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core1",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "18": {
        "mapping": {
          "id": 18,
          "name": "eth1/13/1",
          "controllingPort": 18,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "19": {
        "mapping": {
          "id": 19,
          "name": "eth1/13/5",
          "controllingPort": 19,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core2",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "20": {
        "mapping": {
          "id": 20,
          "name": "eth1/9/1",
          "controllingPort": 20,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "21": {
        "mapping": {
          "id": 21,
          "name": "eth1/9/5",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core3",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "36": {
        "mapping": {
          "id": 36,
          "name": "eth1/25/1",
          "controllingPort": 36,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "37": {
        "mapping": {
          "id": 37,
          "name": "eth1/25/5",
          "controllingPort": 37,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core4",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "38": {
        "mapping": {
          "id": 38,
          "name": "eth1/29/1",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "39": {
        "mapping": {
          "id": 39,
          "name": "eth1/29/5",
          "controllingPort": 39,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core5",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "54": {
        "mapping": {
          "id": 54,
          "name": "eth1/21/1",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "55": {
        "mapping": {
          "id": 55,
          "name": "eth1/21/5",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core6",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "56": {
        "mapping": {
          "id": 56,
          "name": "eth1/17/1",
          "controllingPort": 56,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "57": {
        "mapping": {
          "id": 57,
          "name": "eth1/17/5",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core7",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "72": {
        "mapping": {
          "id": 72,
          "name": "eth1/6/1",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "73": {
        "mapping": {
          "id": 73,
          "name": "eth1/6/5",
          "controllingPort": 73,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core8",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "74": {
        "mapping": {
          "id": 74,
          "name": "eth1/2/1",
          "controllingPort": 74,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "75": {
        "mapping": {
          "id": 75,
          "name": "eth1/2/5",
          "controllingPort": 75,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core9",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "90": {
        "mapping": {
          "id": 90,
          "name": "eth1/10/1",
          "controllingPort": 90,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "91": {
        "mapping": {
          "id": 91,
          "name": "eth1/10/5",
          "controllingPort": 91,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core10",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "92": {
        "mapping": {
          "id": 92,
          "name": "eth1/14/1",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "93": {
        "mapping": {
          "id": 93,
          "name": "eth1/14/5",
          "controllingPort": 93,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core11",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "108": {
        "mapping": {
          "id": 108,
          "name": "eth1/30/1",
          "controllingPort": 108,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "109": {
        "mapping": {
          "id": 109,
          "name": "eth1/30/5",
          "controllingPort": 109,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core12",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "110": {
        "mapping": {
          "id": 110,
          "name": "eth1/26/1",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "111": {
        "mapping": {
          "id": 111,
          "name": "eth1/26/5",
          "controllingPort": 111,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core13",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "126": {
        "mapping": {
          "id": 126,
          "name": "eth1/18/1",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "127": {
        "mapping": {
          "id": 127,
          "name": "eth1/18/5",
          "controllingPort": 127,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core14",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "128": {
        "mapping": {
          "id": 128,
          "name": "eth1/22/1",
          "controllingPort": 128,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "129": {
        "mapping": {
          "id": 129,
          "name": "eth1/22/5",
          "controllingPort": 129,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core15",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "144": {
        "mapping": {
          "id": 144,
          "name": "eth1/3/1",
          "controllingPort": 144,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "145": {
        "mapping": {
          "id": 145,
          "name": "eth1/3/5",
          "controllingPort": 145,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core16",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "146": {
        "mapping": {
          "id": 146,
          "name": "eth1/7/1",
          "controllingPort": 146,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "147": {
        "mapping": {
          "id": 147,
          "name": "eth1/7/5",
          "controllingPort": 147,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core17",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "162": {
        "mapping": {
          "id": 162,
          "name": "eth1/15/1",
          "controllingPort": 162,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "163": {
        "mapping": {
          "id": 163,
          "name": "eth1/15/5",
          "controllingPort": 163,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core18",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "164": {
        "mapping": {
          "id": 164,
          "name": "eth1/11/1",
          "controllingPort": 164,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "165": {
        "mapping": {
          "id": 165,
          "name": "eth1/11/5",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core19",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "180": {
        "mapping": {
          "id": 180,
          "name": "eth1/27/1",
          "controllingPort": 180,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "181": {
        "mapping": {
          "id": 181,
          "name": "eth1/27/5",
          "controllingPort": 181,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core20",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "182": {
        "mapping": {
          "id": 182,
          "name": "eth1/31/1",
          "controllingPort": 182,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "183": {
        "mapping": {
          "id": 183,
          "name": "eth1/31/5",
          "controllingPort": 183,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core21",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "198": {
        "mapping": {
          "id": 198,
          "name": "eth1/23/1",
          "controllingPort": 198,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "199": {
        "mapping": {
          "id": 199,
          "name": "eth1/23/5",
          "controllingPort": 199,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core22",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "200": {
        "mapping": {
          "id": 200,
          "name": "eth1/19/1",
          "controllingPort": 200,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "201": {
        "mapping": {
          "id": 201,
          "name": "eth1/19/5",
          "controllingPort": 201,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core23",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "216": {
        "mapping": {
          "id": 216,
          "name": "eth1/8/1",
          "controllingPort": 216,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "217": {
        "mapping": {
          "id": 217,
          "name": "eth1/8/5",
          "controllingPort": 217,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core24",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "218": {
        "mapping": {
          "id": 218,
          "name": "eth1/4/1",
          "controllingPort": 218,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "219": {
        "mapping": {
          "id": 219,
          "name": "eth1/4/5",
          "controllingPort": 219,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core25",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "234": {
        "mapping": {
          "id": 234,
          "name": "eth1/12/1",
          "controllingPort": 234,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "235": {
        "mapping": {
          "id": 235,
          "name": "eth1/12/5",
          "controllingPort": 235,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core26",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "236": {
        "mapping": {
          "id": 236,
          "name": "eth1/16/1",
          "controllingPort": 236,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "237": {
        "mapping": {
          "id": 237,
          "name": "eth1/16/5",
          "controllingPort": 237,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core27",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "252": {
        "mapping": {
          "id": 252,
          "name": "eth1/32/1",
          "controllingPort": 252,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "253": {
        "mapping": {
          "id": 253,
          "name": "eth1/32/5",
          "controllingPort": 253,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core28",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "254": {
        "mapping": {
          "id": 254,
          "name": "eth1/28/1",
          "controllingPort": 254,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "255": {
        "mapping": {
          "id": 255,
          "name": "eth1/28/5",
          "controllingPort": 255,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core29",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "270": {
        "mapping": {
          "id": 270,
          "name": "eth1/20/1",
          "controllingPort": 270,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "271": {
        "mapping": {
          "id": 271,
          "name": "eth1/20/5",
          "controllingPort": 271,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core30",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "272": {
        "mapping": {
          "id": 272,
          "name": "eth1/24/1",
          "controllingPort": 272,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "273": {
        "mapping": {
          "id": 273,
          "name": "eth1/24/5",
          "controllingPort": 273,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core31",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "288": {
        "mapping": {
          "id": 288,
          "name": "eth1/40/1",
          "controllingPort": 288,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "289": {
        "mapping": {
          "id": 289,
          "name": "eth1/40/5",
          "controllingPort": 289,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core32",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "290": {
        "mapping": {
          "id": 290,
          "name": "eth1/36/1",
          "controllingPort": 290,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "291": {
        "mapping": {
          "id": 291,
          "name": "eth1/36/5",
          "controllingPort": 291,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core33",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "306": {
        "mapping": {
          "id": 306,
          "name": "eth1/44/1",
          "controllingPort": 306,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "307": {
        "mapping": {
          "id": 307,
          "name": "eth1/44/5",
          "controllingPort": 307,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core34",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "308": {
        "mapping": {
          "id": 308,
          "name": "eth1/48/1",
          "controllingPort": 308,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "309": {
        "mapping": {
          "id": 309,
          "name": "eth1/48/5",
          "controllingPort": 309,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core35",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "324": {
        "mapping": {
          "id": 324,
          "name": "eth1/64/1",
          "controllingPort": 324,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "325": {
        "mapping": {
          "id": 325,
          "name": "eth1/64/5",
          "controllingPort": 325,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core36",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "326": {
        "mapping": {
          "id": 326,
          "name": "eth1/60/1",
          "controllingPort": 326,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "327": {
        "mapping": {
          "id": 327,
          "name": "eth1/60/5",
          "controllingPort": 327,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core37",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "342": {
        "mapping": {
          "id": 342,
          "name": "eth1/52/1",
          "controllingPort": 342,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "343": {
        "mapping": {
          "id": 343,
          "name": "eth1/52/5",
          "controllingPort": 343,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core38",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "344": {
        "mapping": {
          "id": 344,
          "name": "eth1/56/1",
          "controllingPort": 344,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "345": {
        "mapping": {
          "id": 345,
          "name": "eth1/56/5",
          "controllingPort": 345,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core39",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "360": {
        "mapping": {
          "id": 360,
          "name": "eth1/35/1",
          "controllingPort": 360,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "361": {
        "mapping": {
          "id": 361,
          "name": "eth1/35/5",
          "controllingPort": 361,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core40",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "362": {
        "mapping": {
          "id": 362,
          "name": "eth1/39/1",
          "controllingPort": 362,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "363": {
        "mapping": {
          "id": 363,
          "name": "eth1/39/5",
          "controllingPort": 363,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core41",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "378": {
        "mapping": {
          "id": 378,
          "name": "eth1/47/1",
          "controllingPort": 378,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "379": {
        "mapping": {
          "id": 379,
          "name": "eth1/47/5",
          "controllingPort": 379,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core42",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "380": {
        "mapping": {
          "id": 380,
          "name": "eth1/43/1",
          "controllingPort": 380,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "381": {
        "mapping": {
          "id": 381,
          "name": "eth1/43/5",
          "controllingPort": 381,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core43",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "396": {
        "mapping": {
          "id": 396,
          "name": "eth1/59/1",
          "controllingPort": 396,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "397": {
        "mapping": {
          "id": 397,
          "name": "eth1/59/5",
          "controllingPort": 397,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core44",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "398": {
        "mapping": {
          "id": 398,
          "name": "eth1/63/1",
          "controllingPort": 398,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "399": {
        "mapping": {
          "id": 399,
          "name": "eth1/63/5",
          "controllingPort": 399,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core45",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "414": {
        "mapping": {
          "id": 414,
          "name": "eth1/55/1",
          "controllingPort": 414,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "415": {
        "mapping": {
          "id": 415,
          "name": "eth1/55/5",
          "controllingPort": 415,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core46",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "416": {
        "mapping": {
          "id": 416,
          "name": "eth1/51/1",
          "controllingPort": 416,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "417": {
        "mapping": {
          "id": 417,
          "name": "eth1/51/5",
          "controllingPort": 417,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core47",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "432": {
        "mapping": {
          "id": 432,
          "name": "eth1/38/1",
          "controllingPort": 432,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "433": {
        "mapping": {
          "id": 433,
          "name": "eth1/38/5",
          "controllingPort": 433,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core48",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "434": {
        "mapping": {
          "id": 434,
          "name": "eth1/34/1",
          "controllingPort": 434,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "435": {
        "mapping": {
          "id": 435,
          "name": "eth1/34/5",
          "controllingPort": 435,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core49",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "450": {
        "mapping": {
          "id": 450,
          "name": "eth1/42/1",
          "controllingPort": 450,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "451": {
        "mapping": {
          "id": 451,
          "name": "eth1/42/5",
          "controllingPort": 451,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core50",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "452": {
        "mapping": {
          "id": 452,
          "name": "eth1/46/1",
          "controllingPort": 452,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "453": {
        "mapping": {
          "id": 453,
          "name": "eth1/46/5",
          "controllingPort": 453,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core51",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "468": {
        "mapping": {
          "id": 468,
          "name": "eth1/62/1",
          "controllingPort": 468,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "469": {
        "mapping": {
          "id": 469,
          "name": "eth1/62/5",
          "controllingPort": 469,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core52",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "470": {
        "mapping": {
          "id": 470,
          "name": "eth1/58/1",
          "controllingPort": 470,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "471": {
        "mapping": {
          "id": 471,
          "name": "eth1/58/5",
          "controllingPort": 471,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core53",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "486": {
        "mapping": {
          "id": 486,
          "name": "eth1/50/1",
          "controllingPort": 486,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "487": {
        "mapping": {
          "id": 487,
          "name": "eth1/50/5",
          "controllingPort": 487,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core54",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "488": {
        "mapping": {
          "id": 488,
          "name": "eth1/54/1",
          "controllingPort": 488,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "489": {
        "mapping": {
          "id": 489,
          "name": "eth1/54/5",
          "controllingPort": 489,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core55",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "504": {
        "mapping": {
          "id": 504,
          "name": "eth1/33/1",
          "controllingPort": 504,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "505": {
        "mapping": {
          "id": 505,
          "name": "eth1/33/5",
          "controllingPort": 505,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core56",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "506": {
        "mapping": {
          "id": 506,
          "name": "eth1/37/1",
          "controllingPort": 506,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "507": {
        "mapping": {
          "id": 507,
          "name": "eth1/37/5",
          "controllingPort": 507,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core57",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "522": {
        "mapping": {
          "id": 522,
          "name": "eth1/45/1",
          "controllingPort": 522,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "523": {
        "mapping": {
          "id": 523,
          "name": "eth1/45/5",
          "controllingPort": 523,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core58",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "524": {
        "mapping": {
          "id": 524,
          "name": "eth1/41/1",
          "controllingPort": 524,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "525": {
        "mapping": {
          "id": 525,
          "name": "eth1/41/5",
          "controllingPort": 525,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core59",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "540": {
        "mapping": {
          "id": 540,
          "name": "eth1/57/1",
          "controllingPort": 540,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "541": {
        "mapping": {
          "id": 541,
          "name": "eth1/57/5",
          "controllingPort": 541,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core60",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "542": {
        "mapping": {
          "id": 542,
          "name": "eth1/61/1",
          "controllingPort": 542,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "543": {
        "mapping": {
          "id": 543,
          "name": "eth1/61/5",
          "controllingPort": 543,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core61",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "558": {
        "mapping": {
          "id": 558,
          "name": "eth1/53/1",
          "controllingPort": 558,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "559": {
        "mapping": {
          "id": 559,
          "name": "eth1/53/5",
          "controllingPort": 559,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core62",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "560": {
        "mapping": {
          "id": 560,
          "name": "eth1/49/1",
          "controllingPort": 560,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "561": {
        "mapping": {
          "id": 561,
          "name": "eth1/49/5",
          "controllingPort": 561,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "52": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH6_NIF-slot1/chip1/core63",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    }
  },
  "chips": [
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core24",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core25",
      "type": 1,
      "physicalID": 25
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core26",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core27",
      "type": 1,
      "physicalID": 27
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core28",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core29",
      "type": 1,
      "physicalID": 29
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core30",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core31",
      "type": 1,
      "physicalID": 31
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core32",
      "type": 1,
      "physicalID": 32
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core33",
      "type": 1,
      "physicalID": 33
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core34",
      "type": 1,
      "physicalID": 34
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core35",
      "type": 1,
      "physicalID": 35
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core36",
      "type": 1,
      "physicalID": 36
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core37",
      "type": 1,
      "physicalID": 37
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core38",
      "type": 1,
      "physicalID": 38
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core39",
      "type": 1,
      "physicalID": 39
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core40",
      "type": 1,
      "physicalID": 40
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core41",
      "type": 1,
      "physicalID": 41
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core42",
      "type": 1,
      "physicalID": 42
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core43",
      "type": 1,
      "physicalID": 43
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core44",
      "type": 1,
      "physicalID": 44
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core45",
      "type": 1,
      "physicalID": 45
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core46",
      "type": 1,
      "physicalID": 46
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core47",
      "type": 1,
      "physicalID": 47
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core48",
      "type": 1,
      "physicalID": 48
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core49",
      "type": 1,
      "physicalID": 49
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core50",
      "type": 1,
      "physicalID": 50
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core51",
      "type": 1,
      "physicalID": 51
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core52",
      "type": 1,
      "physicalID": 52
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core53",
      "type": 1,
      "physicalID": 53
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core54",
      "type": 1,
      "physicalID": 54
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core55",
      "type": 1,
      "physicalID": 55
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core56",
      "type": 1,
      "physicalID": 56
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core57",
      "type": 1,
      "physicalID": 57
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core58",
      "type": 1,
      "physicalID": 58
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core59",
      "type": 1,
      "physicalID": 59
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core60",
      "type": 1,
      "physicalID": 60
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core61",
      "type": 1,
      "physicalID": 61
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core62",
      "type": 1,
      "physicalID": 62
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core63",
      "type": 1,
      "physicalID": 63
    },
    {
      "name": "NPU-TH6_NIF-slot1/chip1/core64",
      "type": 1,
      "physicalID": 64
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip1",
      "type": 3,
      "physicalID": 0
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip2",
      "type": 3,
      "physicalID": 1
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip3",
      "type": 3,
      "physicalID": 2
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip4",
      "type": 3,
      "physicalID": 3
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip5",
      "type": 3,
      "physicalID": 4
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip6",
      "type": 3,
      "physicalID": 5
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip7",
      "type": 3,
      "physicalID": 6
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip8",
      "type": 3,
      "physicalID": 7
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip9",
      "type": 3,
      "physicalID": 8
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip10",
      "type": 3,
      "physicalID": 9
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip11",
      "type": 3,
      "physicalID": 10
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip12",
      "type": 3,
      "physicalID": 11
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip13",
      "type": 3,
      "physicalID": 12
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip14",
      "type": 3,
      "physicalID": 13
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip15",
      "type": 3,
      "physicalID": 14
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip16",
      "type": 3,
      "physicalID": 15
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip17",
      "type": 3,
      "physicalID": 16
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip18",
      "type": 3,
      "physicalID": 17
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip19",
      "type": 3,
      "physicalID": 18
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip20",
      "type": 3,
      "physicalID": 19
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip21",
      "type": 3,
      "physicalID": 20
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip22",
      "type": 3,
      "physicalID": 21
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip23",
      "type": 3,
      "physicalID": 22
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip24",
      "type": 3,
      "physicalID": 23
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip25",
      "type": 3,
      "physicalID": 24
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip26",
      "type": 3,
      "physicalID": 25
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip27",
      "type": 3,
      "physicalID": 26
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip28",
      "type": 3,
      "physicalID": 27
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip29",
      "type": 3,
      "physicalID": 28
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip30",
      "type": 3,
      "physicalID": 29
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip31",
      "type": 3,
      "physicalID": 30
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip32",
      "type": 3,
      "physicalID": 31
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip33",
      "type": 3,
      "physicalID": 32
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip34",
      "type": 3,
      "physicalID": 33
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip35",
      "type": 3,
      "physicalID": 34
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip36",
      "type": 3,
      "physicalID": 35
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip37",
      "type": 3,
      "physicalID": 36
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip38",
      "type": 3,
      "physicalID": 37
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip39",
      "type": 3,
      "physicalID": 38
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip40",
      "type": 3,
      "physicalID": 39
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip41",
      "type": 3,
      "physicalID": 40
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip42",
      "type": 3,
      "physicalID": 41
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip43",
      "type": 3,
      "physicalID": 42
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip44",
      "type": 3,
      "physicalID": 43
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip45",
      "type": 3,
      "physicalID": 44
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip46",
      "type": 3,
      "physicalID": 45
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip47",
      "type": 3,
      "physicalID": 46
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip48",
      "type": 3,
      "physicalID": 47
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip49",
      "type": 3,
      "physicalID": 48
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip50",
      "type": 3,
      "physicalID": 49
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip51",
      "type": 3,
      "physicalID": 50
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip52",
      "type": 3,
      "physicalID": 51
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip53",
      "type": 3,
      "physicalID": 52
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip54",
      "type": 3,
      "physicalID": 53
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip55",
      "type": 3,
      "physicalID": 54
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip56",
      "type": 3,
      "physicalID": 55
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip57",
      "type": 3,
      "physicalID": 56
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip58",
      "type": 3,
      "physicalID": 57
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip59",
      "type": 3,
      "physicalID": 58
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip60",
      "type": 3,
      "physicalID": 59
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip61",
      "type": 3,
      "physicalID": 60
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip62",
      "type": 3,
      "physicalID": 61
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip63",
      "type": 3,
      "physicalID": 62
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip64",
      "type": 3,
      "physicalID": 63
    },
    {
      "name": "TRANSCEIVER-QSFP28-slot1/chip65",
      "type": 3,
      "physicalID": 64
    }
  ],
  "platformSupportedProfiles": [
    {
      "factor": {
        "profileID": 25
      },
      "profile": {
        "speed": 200000,
        "iphy": {
          "numLanes": 4,
          "modulation": 2,
          "fec": 11,
          "medium": 3,
          "interfaceType": 3
        }
      }
    },
    {
      "factor": {
        "profileID": 52
      },
      "profile": {
        "speed": 800000,
        "iphy": {
          "numLanes": 4,
          "modulation": 2,
          "fec": 11,
          "medium": 2,
          "interfaceType": 4
        }
      }
    },
    {
      "factor": {
        "profileID": 38
      },
      "profile": {
        "speed": 400000,
        "iphy": {
          "numLanes": 4,
          "modulation": 2,
          "fec": 11,
          "medium": 3,
          "interfaceType": 3
        }
      }
    }
  ]
})";
} // namespace

namespace facebook::fboss {
Icecube800bcPlatformMapping::Icecube800bcPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Icecube800bcPlatformMapping::Icecube800bcPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
