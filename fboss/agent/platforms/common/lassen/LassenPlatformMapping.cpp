/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/lassen/LassenPlatformMapping.h"

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
                "chip": "IFG9",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 7
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "9": {
        "mapping": {
          "id": 9,
          "name": "eth1/2/1",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "IFG9",
                "lane": 16
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                10,
                11,
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                10
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                10,
                11,
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                10,
                11,
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "10": {
        "mapping": {
          "id": 10,
          "name": "eth1/2/2",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "IFG9",
                "lane": 17
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "11": {
        "mapping": {
          "id": 11,
          "name": "eth1/2/3",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "IFG9",
                "lane": 18
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "12": {
        "mapping": {
          "id": 12,
          "name": "eth1/2/4",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "IFG9",
                "lane": 19
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "17": {
        "mapping": {
          "id": 17,
          "name": "eth1/3/1",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "IFG9",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                18,
                19,
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                18
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                18,
                19,
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                18,
                19,
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 8
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 9
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 10
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 11
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
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
          "name": "eth1/3/2",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "IFG9",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
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
          "name": "eth1/3/3",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "IFG9",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
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
          "name": "eth1/3/4",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "IFG9",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "25": {
        "mapping": {
          "id": 25,
          "name": "eth1/4/1",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "IFG8",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                26,
                27,
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                26
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                26,
                27,
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                26,
                27,
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 8
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 9
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 10
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 11
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "26": {
        "mapping": {
          "id": 26,
          "name": "eth1/4/2",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "IFG8",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "27": {
        "mapping": {
          "id": 27,
          "name": "eth1/4/3",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "IFG8",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "28": {
        "mapping": {
          "id": 28,
          "name": "eth1/4/4",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "IFG8",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "33": {
        "mapping": {
          "id": 33,
          "name": "eth1/5/1",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "IFG7",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 7
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "41": {
        "mapping": {
          "id": 41,
          "name": "eth1/6/1",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "IFG8",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                42,
                43,
                44
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                42
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                42,
                43,
                44
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                42,
                43,
                44
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "42": {
        "mapping": {
          "id": 42,
          "name": "eth1/6/2",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "IFG8",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "43": {
        "mapping": {
          "id": 43,
          "name": "eth1/6/3",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "IFG8",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                44
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "44": {
        "mapping": {
          "id": 44,
          "name": "eth1/6/4",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "IFG8",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "49": {
        "mapping": {
          "id": 49,
          "name": "eth1/7/1",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                50,
                51,
                52
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                50
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                50,
                51,
                52
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                50,
                51,
                52
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "50": {
        "mapping": {
          "id": 50,
          "name": "eth1/7/2",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "51": {
        "mapping": {
          "id": 51,
          "name": "eth1/7/3",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                52
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "52": {
        "mapping": {
          "id": 52,
          "name": "eth1/7/4",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
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
          "name": "eth1/8/1",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                58,
                59,
                60
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                58
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                58,
                59,
                60
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                58,
                59,
                60
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 8
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 9
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 10
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 11
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "58": {
        "mapping": {
          "id": 58,
          "name": "eth1/8/2",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "59": {
        "mapping": {
          "id": 59,
          "name": "eth1/8/3",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                60
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "60": {
        "mapping": {
          "id": 60,
          "name": "eth1/8/4",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "65": {
        "mapping": {
          "id": 65,
          "name": "eth1/9/1",
          "controllingPort": 65,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 16
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 17
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 18
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 19
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 20
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 21
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 22
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 23
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 7
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 20
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 21
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 22
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 23
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 7
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
          "name": "eth1/10/1",
          "controllingPort": 73,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                74,
                75,
                76
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                74
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                74,
                75,
                76
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                74,
                75,
                76
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
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
          "name": "eth1/10/2",
          "controllingPort": 73,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
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
          "name": "eth1/10/3",
          "controllingPort": 73,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                76
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "76": {
        "mapping": {
          "id": 76,
          "name": "eth1/10/4",
          "controllingPort": 73,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "81": {
        "mapping": {
          "id": 81,
          "name": "eth1/11/1",
          "controllingPort": 81,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                82,
                83,
                84
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                82
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                82,
                83,
                84
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                82,
                83,
                84
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "82": {
        "mapping": {
          "id": 82,
          "name": "eth1/11/2",
          "controllingPort": 81,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "83": {
        "mapping": {
          "id": 83,
          "name": "eth1/11/3",
          "controllingPort": 81,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                84
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "84": {
        "mapping": {
          "id": 84,
          "name": "eth1/11/4",
          "controllingPort": 81,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "89": {
        "mapping": {
          "id": 89,
          "name": "eth1/12/1",
          "controllingPort": 89,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 16
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                90,
                91,
                92
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                90
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                90,
                91,
                92
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                90,
                91,
                92
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
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
          "name": "eth1/12/2",
          "controllingPort": 89,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 17
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
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
          "name": "eth1/12/3",
          "controllingPort": 89,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 18
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                92
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
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
          "name": "eth1/12/4",
          "controllingPort": 89,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 19
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "97": {
        "mapping": {
          "id": 97,
          "name": "eth1/13/1",
          "controllingPort": 97,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 7
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -80,
                      "pre2": 0,
                      "main": 880,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "105": {
        "mapping": {
          "id": 105,
          "name": "eth1/14/1",
          "controllingPort": 105,
          "pins": [
            {
              "a": {
                "chip": "IFG7",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                106,
                107,
                108
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                106
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                106,
                107,
                108
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                106,
                107,
                108
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 8
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 9
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 10
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 11
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "106": {
        "mapping": {
          "id": 106,
          "name": "eth1/14/2",
          "controllingPort": 105,
          "pins": [
            {
              "a": {
                "chip": "IFG7",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "107": {
        "mapping": {
          "id": 107,
          "name": "eth1/14/3",
          "controllingPort": 105,
          "pins": [
            {
              "a": {
                "chip": "IFG7",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                108
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
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
          "name": "eth1/14/4",
          "controllingPort": 105,
          "pins": [
            {
              "a": {
                "chip": "IFG7",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "113": {
        "mapping": {
          "id": 113,
          "name": "eth1/15/1",
          "controllingPort": 113,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 16
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                114,
                115,
                116
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                114
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                114,
                115,
                116
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                114,
                115,
                116
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "114": {
        "mapping": {
          "id": 114,
          "name": "eth1/15/2",
          "controllingPort": 113,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 17
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "115": {
        "mapping": {
          "id": 115,
          "name": "eth1/15/3",
          "controllingPort": 113,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 18
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                116
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "116": {
        "mapping": {
          "id": 116,
          "name": "eth1/15/4",
          "controllingPort": 113,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 19
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "121": {
        "mapping": {
          "id": 121,
          "name": "eth1/16/1",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                122,
                123,
                124
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                122
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                122,
                123,
                124
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                122,
                123,
                124
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 8
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 9
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 10
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 11
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "122": {
        "mapping": {
          "id": 122,
          "name": "eth1/16/2",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "123": {
        "mapping": {
          "id": 123,
          "name": "eth1/16/3",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                124
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "124": {
        "mapping": {
          "id": 124,
          "name": "eth1/16/4",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
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
          "name": "eth1/17/1",
          "controllingPort": 129,
          "pins": [
            {
              "a": {
                "chip": "IFG5",
                "lane": 16
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 17
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 18
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 19
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 20
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 21
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 22
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 23
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 7
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 20
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 21
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 22
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 23
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "137": {
        "mapping": {
          "id": 137,
          "name": "eth1/18/1",
          "controllingPort": 137,
          "pins": [
            {
              "a": {
                "chip": "IFG5",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                138,
                139,
                140
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                138
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                138,
                139,
                140
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                138,
                139,
                140
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 8
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 9
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 10
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 11
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "138": {
        "mapping": {
          "id": 138,
          "name": "eth1/18/2",
          "controllingPort": 137,
          "pins": [
            {
              "a": {
                "chip": "IFG5",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "139": {
        "mapping": {
          "id": 139,
          "name": "eth1/18/3",
          "controllingPort": 137,
          "pins": [
            {
              "a": {
                "chip": "IFG5",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                140
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "140": {
        "mapping": {
          "id": 140,
          "name": "eth1/18/4",
          "controllingPort": 137,
          "pins": [
            {
              "a": {
                "chip": "IFG5",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
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
          "name": "eth1/19/1",
          "controllingPort": 145,
          "pins": [
            {
              "a": {
                "chip": "IFG5",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                146,
                147,
                148
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                146
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                146,
                147,
                148
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                146,
                147,
                148
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
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
          "name": "eth1/19/2",
          "controllingPort": 145,
          "pins": [
            {
              "a": {
                "chip": "IFG5",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
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
          "name": "eth1/19/3",
          "controllingPort": 145,
          "pins": [
            {
              "a": {
                "chip": "IFG5",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                148
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "148": {
        "mapping": {
          "id": 148,
          "name": "eth1/19/4",
          "controllingPort": 145,
          "pins": [
            {
              "a": {
                "chip": "IFG5",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "153": {
        "mapping": {
          "id": 153,
          "name": "eth1/20/1",
          "controllingPort": 153,
          "pins": [
            {
              "a": {
                "chip": "IFG4",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                154,
                155,
                156
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                154
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                154,
                155,
                156
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                154,
                155,
                156
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 8
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 9
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 10
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 11
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "154": {
        "mapping": {
          "id": 154,
          "name": "eth1/20/2",
          "controllingPort": 153,
          "pins": [
            {
              "a": {
                "chip": "IFG4",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "155": {
        "mapping": {
          "id": 155,
          "name": "eth1/20/3",
          "controllingPort": 153,
          "pins": [
            {
              "a": {
                "chip": "IFG4",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                156
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "156": {
        "mapping": {
          "id": 156,
          "name": "eth1/20/4",
          "controllingPort": 153,
          "pins": [
            {
              "a": {
                "chip": "IFG4",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "161": {
        "mapping": {
          "id": 161,
          "name": "eth1/21/1",
          "controllingPort": 161,
          "pins": [
            {
              "a": {
                "chip": "IFG1",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 12
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 13
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 14
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 15
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 7
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 880,
                      "post": -60,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 880,
                      "post": -60,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 880,
                      "post": -60,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 880,
                      "post": -60,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 880,
                      "post": -60,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 880,
                      "post": -60,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 880,
                      "post": -60,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 880,
                      "post": -60,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 12
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 880,
                      "post": -60,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 13
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 880,
                      "post": -60,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 14
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 880,
                      "post": -60,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 15
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 880,
                      "post": -60,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "169": {
        "mapping": {
          "id": 169,
          "name": "eth1/22/1",
          "controllingPort": 169,
          "pins": [
            {
              "a": {
                "chip": "IFG1",
                "lane": 16
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                170,
                171,
                172
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                170
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                170,
                171,
                172
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                170,
                171,
                172
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "170": {
        "mapping": {
          "id": 170,
          "name": "eth1/22/2",
          "controllingPort": 169,
          "pins": [
            {
              "a": {
                "chip": "IFG1",
                "lane": 17
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "171": {
        "mapping": {
          "id": 171,
          "name": "eth1/22/3",
          "controllingPort": 169,
          "pins": [
            {
              "a": {
                "chip": "IFG1",
                "lane": 18
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                172
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "172": {
        "mapping": {
          "id": 172,
          "name": "eth1/22/4",
          "controllingPort": 169,
          "pins": [
            {
              "a": {
                "chip": "IFG1",
                "lane": 19
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "177": {
        "mapping": {
          "id": 177,
          "name": "eth1/23/1",
          "controllingPort": 177,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 16
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                178,
                179,
                180
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                178
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                178,
                179,
                180
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                178,
                179,
                180
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 16
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 17
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 18
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 19
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "178": {
        "mapping": {
          "id": 178,
          "name": "eth1/23/2",
          "controllingPort": 177,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 17
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "179": {
        "mapping": {
          "id": 179,
          "name": "eth1/23/3",
          "controllingPort": 177,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 18
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                180
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
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
          "name": "eth1/23/4",
          "controllingPort": 177,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 19
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "185": {
        "mapping": {
          "id": 185,
          "name": "eth1/24/1",
          "controllingPort": 185,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                186,
                187,
                188
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                186
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                186,
                187,
                188
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                186,
                187,
                188
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "186": {
        "mapping": {
          "id": 186,
          "name": "eth1/24/2",
          "controllingPort": 185,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "187": {
        "mapping": {
          "id": 187,
          "name": "eth1/24/3",
          "controllingPort": 185,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                188
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "188": {
        "mapping": {
          "id": 188,
          "name": "eth1/24/4",
          "controllingPort": 185,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "193": {
        "mapping": {
          "id": 193,
          "name": "eth1/25/1",
          "controllingPort": 193,
          "pins": [
            {
              "a": {
                "chip": "IFG1",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 7
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -150,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 7
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
          "name": "eth1/26/1",
          "controllingPort": 201,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                202,
                203,
                204
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                202
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                202,
                203,
                204
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                202,
                203,
                204
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 8
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 9
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 10
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 11
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "202": {
        "mapping": {
          "id": 202,
          "name": "eth1/26/2",
          "controllingPort": 201,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "203": {
        "mapping": {
          "id": 203,
          "name": "eth1/26/3",
          "controllingPort": 201,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                204
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "204": {
        "mapping": {
          "id": 204,
          "name": "eth1/26/4",
          "controllingPort": 201,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "209": {
        "mapping": {
          "id": 209,
          "name": "eth1/27/1",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "IFG4",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                210,
                211,
                212
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                210
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                210,
                211,
                212
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                210,
                211,
                212
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "210": {
        "mapping": {
          "id": 210,
          "name": "eth1/27/2",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "IFG4",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "211": {
        "mapping": {
          "id": 211,
          "name": "eth1/27/3",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "IFG4",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                212
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "212": {
        "mapping": {
          "id": 212,
          "name": "eth1/27/4",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "IFG4",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
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
          "name": "eth1/28/1",
          "controllingPort": 217,
          "pins": [
            {
              "a": {
                "chip": "IFG3",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                218,
                219,
                220
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                218
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                218,
                219,
                220
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                218,
                219,
                220
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
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
          "name": "eth1/28/2",
          "controllingPort": 217,
          "pins": [
            {
              "a": {
                "chip": "IFG3",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
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
          "name": "eth1/28/3",
          "controllingPort": 217,
          "pins": [
            {
              "a": {
                "chip": "IFG3",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                220
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "220": {
        "mapping": {
          "id": 220,
          "name": "eth1/28/4",
          "controllingPort": 217,
          "pins": [
            {
              "a": {
                "chip": "IFG3",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "225": {
        "mapping": {
          "id": 225,
          "name": "eth1/29/1",
          "controllingPort": 225,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 12
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 13
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 14
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 15
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 7
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 12
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 13
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 14
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 15
                    },
                    "tx": {
                      "pre": -25,
                      "pre2": 0,
                      "main": 700,
                      "post": -175,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 12
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 13
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 14
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 15
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 800,
                      "post": -100,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 2,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 12
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 13
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 14
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 15
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 12
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 13
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 14
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 15
                    },
                    "tx": {
                      "pre": -60,
                      "pre2": 0,
                      "main": 900,
                      "post": -40,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 7,
                      "dspMode": 0,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "233": {
        "mapping": {
          "id": 233,
          "name": "eth1/30/1",
          "controllingPort": 233,
          "pins": [
            {
              "a": {
                "chip": "IFG3",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                234,
                235,
                236
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                234
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                234,
                235,
                236
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                234,
                235,
                236
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 8
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 9
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 10
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 11
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 750,
                      "post": -200,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
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
          "name": "eth1/30/2",
          "controllingPort": 233,
          "pins": [
            {
              "a": {
                "chip": "IFG3",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
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
          "name": "eth1/30/3",
          "controllingPort": 233,
          "pins": [
            {
              "a": {
                "chip": "IFG3",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                236
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
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
          "name": "eth1/30/4",
          "controllingPort": 233,
          "pins": [
            {
              "a": {
                "chip": "IFG3",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "241": {
        "mapping": {
          "id": 241,
          "name": "eth1/31/1",
          "controllingPort": 241,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 4
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 4
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 4
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 4
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                242,
                243,
                244
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                242
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 5
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                242,
                243,
                244
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                242,
                243,
                244
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "242": {
        "mapping": {
          "id": 242,
          "name": "eth1/31/2",
          "controllingPort": 241,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 5
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 5
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 5
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "243": {
        "mapping": {
          "id": 243,
          "name": "eth1/31/3",
          "controllingPort": 241,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 6
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 6
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 6
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 6
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                244
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "244": {
        "mapping": {
          "id": 244,
          "name": "eth1/31/4",
          "controllingPort": 241,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 7
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "249": {
        "mapping": {
          "id": 249,
          "name": "eth1/32/1",
          "controllingPort": 249,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 16
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                250,
                251,
                252
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                250
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "22": {
              "subsumedPorts": [
                250,
                251,
                252
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "24": {
              "subsumedPorts": [
                250,
                251,
                252
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -120,
                      "pre2": 0,
                      "main": 700,
                      "post": -160,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 2,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "250": {
        "mapping": {
          "id": 250,
          "name": "eth1/32/2",
          "controllingPort": 249,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 17
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "251": {
        "mapping": {
          "id": 251,
          "name": "eth1/32/3",
          "controllingPort": 249,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 18
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "21": {
              "subsumedPorts": [
                252
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
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
          "name": "eth1/32/4",
          "controllingPort": 249,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 19
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -75,
                      "pre2": 0,
                      "main": 800,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 59,
                      "dspMode": 4,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -180,
                      "pre2": 0,
                      "main": 600,
                      "post": -140,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
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
      "name": "IFG0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "IFG1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "IFG2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "IFG3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "IFG4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "IFG5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "IFG6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "IFG7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "IFG8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "IFG9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "IFG10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "IFG11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "eth1/1",
      "type": 3,
      "physicalID": 0
    },
    {
      "name": "eth1/2",
      "type": 3,
      "physicalID": 1
    },
    {
      "name": "eth1/3",
      "type": 3,
      "physicalID": 2
    },
    {
      "name": "eth1/4",
      "type": 3,
      "physicalID": 3
    },
    {
      "name": "eth1/5",
      "type": 3,
      "physicalID": 4
    },
    {
      "name": "eth1/6",
      "type": 3,
      "physicalID": 5
    },
    {
      "name": "eth1/7",
      "type": 3,
      "physicalID": 6
    },
    {
      "name": "eth1/8",
      "type": 3,
      "physicalID": 7
    },
    {
      "name": "eth1/9",
      "type": 3,
      "physicalID": 8
    },
    {
      "name": "eth1/10",
      "type": 3,
      "physicalID": 9
    },
    {
      "name": "eth1/11",
      "type": 3,
      "physicalID": 10
    },
    {
      "name": "eth1/12",
      "type": 3,
      "physicalID": 11
    },
    {
      "name": "eth1/13",
      "type": 3,
      "physicalID": 12
    },
    {
      "name": "eth1/14",
      "type": 3,
      "physicalID": 13
    },
    {
      "name": "eth1/15",
      "type": 3,
      "physicalID": 14
    },
    {
      "name": "eth1/16",
      "type": 3,
      "physicalID": 15
    },
    {
      "name": "eth1/17",
      "type": 3,
      "physicalID": 16
    },
    {
      "name": "eth1/18",
      "type": 3,
      "physicalID": 17
    },
    {
      "name": "eth1/19",
      "type": 3,
      "physicalID": 18
    },
    {
      "name": "eth1/20",
      "type": 3,
      "physicalID": 19
    },
    {
      "name": "eth1/21",
      "type": 3,
      "physicalID": 20
    },
    {
      "name": "eth1/22",
      "type": 3,
      "physicalID": 21
    },
    {
      "name": "eth1/23",
      "type": 3,
      "physicalID": 22
    },
    {
      "name": "eth1/24",
      "type": 3,
      "physicalID": 23
    },
    {
      "name": "eth1/25",
      "type": 3,
      "physicalID": 24
    },
    {
      "name": "eth1/26",
      "type": 3,
      "physicalID": 25
    },
    {
      "name": "eth1/27",
      "type": 3,
      "physicalID": 26
    },
    {
      "name": "eth1/28",
      "type": 3,
      "physicalID": 27
    },
    {
      "name": "eth1/29",
      "type": 3,
      "physicalID": 28
    },
    {
      "name": "eth1/30",
      "type": 3,
      "physicalID": 29
    },
    {
      "name": "eth1/31",
      "type": 3,
      "physicalID": 30
    },
    {
      "name": "eth1/32",
      "type": 3,
      "physicalID": 31
    }
  ],
  "platformSettings": {
    "1": "/dev/uio0"
  },
  "platformSupportedProfiles": [
    {
      "factor": {
        "profileID": 11
      },
      "profile": {
        "speed": 10000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 10,
          "interfaceType": 10
        }
      }
    },
    {
      "factor": {
        "profileID": 14
      },
      "profile": {
        "speed": 25000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 10,
          "interfaceType": 10
        }
      }
    },
    {
      "factor": {
        "profileID": 15
      },
      "profile": {
        "speed": 25000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 74,
          "medium": 1,
          "interfaceMode": 10,
          "interfaceType": 10
        }
      }
    },
    {
      "factor": {
        "profileID": 17
      },
      "profile": {
        "speed": 40000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 12,
          "interfaceType": 12
        }
      }
    },
    {
      "factor": {
        "profileID": 18
      },
      "profile": {
        "speed": 40000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 40,
          "interfaceType": 40
        }
      }
    },
    {
      "factor": {
        "profileID": 19
      },
      "profile": {
        "speed": 50000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 11,
          "interfaceType": 11
        }
      }
    },
    {
      "factor": {
        "profileID": 21
      },
      "profile": {
        "speed": 50000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 528,
          "medium": 1,
          "interfaceMode": 11,
          "interfaceType": 11
        }
      }
    },
    {
      "factor": {
        "profileID": 22
      },
      "profile": {
        "speed": 100000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 528,
          "medium": 1,
          "interfaceMode": 12,
          "interfaceType": 12
        }
      }
    },
    {
      "factor": {
        "profileID": 23
      },
      "profile": {
        "speed": 100000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 528,
          "medium": 2,
          "interfaceMode": 3,
          "interfaceType": 3
        }
      }
    },
    {
      "factor": {
        "profileID": 24
      },
      "profile": {
        "speed": 200000,
        "iphy": {
          "numLanes": 4,
          "modulation": 2,
          "fec": 11,
          "medium": 1,
          "interfaceMode": 12,
          "interfaceType": 12
        }
      }
    },
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
          "medium": 2,
          "interfaceMode": 3,
          "interfaceType": 3
        }
      }
    },
    {
      "factor": {
        "profileID": 26
      },
      "profile": {
        "speed": 400000,
        "iphy": {
          "numLanes": 8,
          "modulation": 2,
          "fec": 11,
          "medium": 2,
          "interfaceMode": 4,
          "interfaceType": 4
        }
      }
    }
  ]
}
)";
} // namespace

namespace facebook {
namespace fboss {
LassenPlatformMapping::LassenPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

LassenPlatformMapping::LassenPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace fboss
} // namespace facebook
