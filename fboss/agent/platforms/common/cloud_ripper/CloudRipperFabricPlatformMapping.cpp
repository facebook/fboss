/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/cloud_ripper/CloudRipperFabricPlatformMapping.h"

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
                "chip": "IFG10",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/1",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/1",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/1",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/1",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/1",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/1",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/1",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/1",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 10,
          "attachedCorePortIndex": 8
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
                      "afeTrim": 4,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 7
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
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY0",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY0",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY0",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY0",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY0",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY0",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY0",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY0",
                      "lane": 16
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
                "chip": "IFG10",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY1",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY1",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/2",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY1",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY1",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/2",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY1",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY1",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/2",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY1",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY1",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/2",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY1",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY1",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/2",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY1",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY1",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/2",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY1",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY1",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/2",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY1",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY1",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/2",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 10,
          "attachedCorePortIndex": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY1",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY1",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY1",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY1",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY1",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY1",
                      "lane": 16
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
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY2",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY2",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/3",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY2",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY2",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/3",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY2",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY2",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/3",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY2",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY2",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/3",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY2",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY2",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/3",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY2",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY2",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/3",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY2",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY2",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/3",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY2",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY2",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/3",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 9,
          "attachedCorePortIndex": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY2",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY2",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY2",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY2",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY2",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY2",
                      "lane": 16
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
                "chip": "IFG10",
                "lane": 16
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/4",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 17
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/4",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 18
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/4",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 19
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/4",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 20
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/4",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 21
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/4",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 22
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/4",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 23
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/4",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 10,
          "attachedCorePortIndex": 16
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG10",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
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
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY3",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY3",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY3",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY3",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY3",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY3",
                      "lane": 16
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
                "chip": "IFG9",
                "lane": 16
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY4",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY4",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/5",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 17
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY4",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY4",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/5",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 18
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY4",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY4",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/5",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 19
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY4",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY4",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/5",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 20
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY4",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY4",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/5",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 21
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY4",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY4",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/5",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 22
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY4",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY4",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/5",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 23
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY4",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY4",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/5",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 9,
          "attachedCorePortIndex": 16
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 7
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
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY4",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY4",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY4",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY4",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY4",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY4",
                      "lane": 16
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
                "chip": "IFG9",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY5",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY5",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/6",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY5",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY5",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/6",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY5",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY5",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/6",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY5",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY5",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/6",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY5",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY5",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/6",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY5",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY5",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/6",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY5",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY5",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/6",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY5",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY5",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/6",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 9,
          "attachedCorePortIndex": 8
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG9",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
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
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY5",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY5",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY5",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY5",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY5",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY5",
                      "lane": 16
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
                "chip": "IFG8",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/7",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/7",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/7",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/7",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/7",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/7",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/7",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/7",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 8,
          "attachedCorePortIndex": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -56,
                      "pre2": 0,
                      "main": 888,
                      "post": -56,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -56,
                      "pre2": 0,
                      "main": 888,
                      "post": -56,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -56,
                      "pre2": 0,
                      "main": 888,
                      "post": -56,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -56,
                      "pre2": 0,
                      "main": 888,
                      "post": -56,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY6",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY6",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY6",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY6",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY6",
                      "lane": 15
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
                "chip": "IFG8",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY7",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY7",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/8",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY7",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY7",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/8",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY7",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY7",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/8",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY7",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY7",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/8",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY7",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY7",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/8",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY7",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY7",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/8",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY7",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY7",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/8",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY7",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY7",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/8",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 8,
          "attachedCorePortIndex": 8
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG8",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY7",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY7",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY7",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY7",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY7",
                      "lane": 15
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
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY8",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY8",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/9",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY8",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY8",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/9",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY8",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY8",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/9",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY8",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY8",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/9",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY8",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY8",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/9",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY8",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY8",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/9",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY8",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY8",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/9",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY8",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY8",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/9",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 11,
          "attachedCorePortIndex": 8
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 7
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
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY8",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY8",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY8",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY8",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY8",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY8",
                      "lane": 16
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
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/10",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/10",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/10",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/10",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/10",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/10",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/10",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/10",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 11,
          "attachedCorePortIndex": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
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
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY9",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY9",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY9",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY9",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY9",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY9",
                      "lane": 16
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
                "chip": "IFG7",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY10",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY10",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/11",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY10",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY10",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/11",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY10",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY10",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/11",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY10",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY10",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/11",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY10",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY10",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/11",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY10",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY10",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/11",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY10",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY10",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/11",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY10",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY10",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/11",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 7,
          "attachedCorePortIndex": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -56,
                      "pre2": 0,
                      "main": 888,
                      "post": -56,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY10",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY10",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY10",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY10",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY10",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY10",
                      "lane": 16
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
                "chip": "IFG11",
                "lane": 16
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY11",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY11",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/12",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 17
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY11",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY11",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/12",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 18
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY11",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY11",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/12",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 19
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY11",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY11",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/12",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 20
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY11",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY11",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/12",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 21
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY11",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY11",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/12",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 22
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY11",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY11",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/12",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 23
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY11",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY11",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/12",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 11,
          "attachedCorePortIndex": 16
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "lane": 17
                    },
                    "tx": {
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "lane": 18
                    },
                    "tx": {
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "lane": 19
                    },
                    "tx": {
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
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
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY11",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY11",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY11",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY11",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY11",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY11",
                      "lane": 16
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
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/13",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/13",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/13",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/13",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/13",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/13",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/13",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/13",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 6,
          "attachedCorePortIndex": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 7
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
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY12",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY12",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY12",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY12",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY12",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY12",
                      "lane": 16
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
                "junction": {
                  "system": {
                    "chip": "XPHY13",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY13",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/14",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY13",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY13",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/14",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY13",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY13",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/14",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY13",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY13",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/14",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY13",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY13",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/14",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY13",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY13",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/14",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY13",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY13",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/14",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY13",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY13",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/14",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 7,
          "attachedCorePortIndex": 8
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
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
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY13",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY13",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY13",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY13",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY13",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY13",
                      "lane": 16
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
                "junction": {
                  "system": {
                    "chip": "XPHY14",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY14",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/15",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 17
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY14",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY14",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/15",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 18
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY14",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY14",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/15",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 19
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY14",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY14",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/15",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 20
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY14",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY14",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/15",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 21
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY14",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY14",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/15",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 22
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY14",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY14",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/15",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 23
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY14",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY14",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/15",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 6,
          "attachedCorePortIndex": 16
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 17
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 18
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 19
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY14",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY14",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY14",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY14",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY14",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY14",
                      "lane": 16
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
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/16",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/16",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/16",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/16",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/16",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/16",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/16",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/16",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 6,
          "attachedCorePortIndex": 8
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
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
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY15",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY15",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY15",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY15",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY15",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY15",
                      "lane": 16
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
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY16",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY16",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/17",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY16",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY16",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/17",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY16",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY16",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/17",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY16",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY16",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/17",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY16",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY16",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/17",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY16",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY16",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/17",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY16",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY16",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/17",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY16",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY16",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/17",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 5,
          "attachedCorePortIndex": 8
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY16",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY16",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY16",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY16",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY16",
                      "lane": 15
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
                "lane": 16
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY17",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY17",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/18",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 17
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY17",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY17",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/18",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 18
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY17",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY17",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/18",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 19
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY17",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY17",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/18",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 20
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY17",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY17",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/18",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 21
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY17",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY17",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/18",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 22
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY17",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY17",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/18",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 23
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY17",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY17",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/18",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 5,
          "attachedCorePortIndex": 16
        },
        "supportedProfiles": {
          "25": {
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
                      "main": 900,
                      "post": -50,
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
                      "lane": 17
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
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
                      "lane": 18
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
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
                      "lane": 19
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY17",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY17",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY17",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY17",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY17",
                      "lane": 15
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
                "chip": "IFG4",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/19",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/19",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/19",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/19",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/19",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/19",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/19",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/19",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 4,
          "attachedCorePortIndex": 8
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -47,
                      "pre2": 0,
                      "main": 906,
                      "post": -47,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY18",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY18",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY18",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY18",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY18",
                      "lane": 15
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
                "chip": "IFG5",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY19",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY19",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/20",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY19",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY19",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/20",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY19",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY19",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/20",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY19",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY19",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/20",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY19",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY19",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/20",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY19",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY19",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/20",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY19",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY19",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/20",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY19",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY19",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/20",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 5,
          "attachedCorePortIndex": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 900,
                      "post": -50,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY19",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY19",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY19",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY19",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY19",
                      "lane": 15
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
                "chip": "IFG0",
                "lane": 16
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/21",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 17
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/21",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 18
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/21",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 19
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/21",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 20
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/21",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 21
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/21",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 22
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/21",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 23
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/21",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 16
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY20",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY20",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY20",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY20",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY20",
                      "lane": 15
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
                "chip": "IFG4",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/22",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/22",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/22",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/22",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/22",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/22",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/22",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/22",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 4,
          "attachedCorePortIndex": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -56,
                      "pre2": 0,
                      "main": 888,
                      "post": -56,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -56,
                      "pre2": 0,
                      "main": 888,
                      "post": -56,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -56,
                      "pre2": 0,
                      "main": 888,
                      "post": -56,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -56,
                      "pre2": 0,
                      "main": 888,
                      "post": -56,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY21",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY21",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY21",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY21",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY21",
                      "lane": 15
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
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY22",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY22",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/23",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY22",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY22",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/23",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY22",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY22",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/23",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY22",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY22",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/23",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY22",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY22",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/23",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY22",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY22",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/23",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY22",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY22",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/23",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY22",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY22",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/23",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY22",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY22",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY22",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY22",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY22",
                      "lane": 15
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
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/24",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/24",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/24",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/24",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/24",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/24",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/24",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/24",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 8
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG0",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY23",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY23",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY23",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY23",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY23",
                      "lane": 15
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
                "chip": "IFG3",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/25",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/25",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/25",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/25",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/25",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/25",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/25",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/25",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 3,
          "attachedCorePortIndex": 8
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -56,
                      "pre2": 0,
                      "main": 888,
                      "post": -56,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -56,
                      "pre2": 0,
                      "main": 888,
                      "post": -56,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -56,
                      "pre2": 0,
                      "main": 888,
                      "post": -56,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -56,
                      "pre2": 0,
                      "main": 888,
                      "post": -56,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 7
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
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY24",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY24",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY24",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY24",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY24",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY24",
                      "lane": 16
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
                "chip": "IFG3",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY25",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY25",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/26",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY25",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY25",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/26",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY25",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY25",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/26",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY25",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY25",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/26",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY25",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY25",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/26",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY25",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY25",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/26",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY25",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY25",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/26",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY25",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY25",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/26",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 3,
          "attachedCorePortIndex": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
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
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 4
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY25",
                      "lane": 7
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY25",
                      "lane": 4
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY25",
                      "lane": 19
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY25",
                      "lane": 18
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY25",
                      "lane": 17
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY25",
                      "lane": 16
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
                "chip": "IFG2",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/27",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/27",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/27",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/27",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/27",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/27",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/27",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/27",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 2,
          "attachedCorePortIndex": 8
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -64,
                      "pre2": 0,
                      "main": 868,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY26",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY26",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY26",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY26",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY26",
                      "lane": 15
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
                "chip": "IFG2",
                "lane": 16
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/28",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 17
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/28",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 18
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/28",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 19
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/28",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 20
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/28",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 21
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/28",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 22
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/28",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 23
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/28",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 2,
          "attachedCorePortIndex": 16
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
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
                      "pre": -70,
                      "pre2": 0,
                      "main": 855,
                      "post": -75,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY27",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY27",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY27",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY27",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY27",
                      "lane": 15
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
                "chip": "IFG1",
                "lane": 16
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY28",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY28",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/29",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 17
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY28",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY28",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/29",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 18
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY28",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY28",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/29",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 19
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY28",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY28",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/29",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 20
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY28",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY28",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/29",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 21
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY28",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY28",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/29",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 22
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY28",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY28",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/29",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 23
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY28",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY28",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/29",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 16
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 16
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
                      "dspMode": 7,
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
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY28",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY28",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY28",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY28",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY28",
                      "lane": 15
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
                "chip": "IFG2",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/30",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/30",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/30",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/30",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/30",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/30",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/30",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/30",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 2,
          "attachedCorePortIndex": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY29",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY29",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY29",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY29",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY29",
                      "lane": 15
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
                "chip": "IFG1",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY30",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY30",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/31",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY30",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY30",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/31",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY30",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY30",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/31",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY30",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY30",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/31",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY30",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY30",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/31",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY30",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY30",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/31",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY30",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY30",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/31",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY30",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY30",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/31",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 0
        },
        "supportedProfiles": {
          "25": {
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
                      "main": 825,
                      "post": -125,
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
                      "lane": 1
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "lane": 2
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
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
                      "lane": 3
                    },
                    "tx": {
                      "pre": -50,
                      "pre2": 0,
                      "main": 825,
                      "post": -125,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 63,
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
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY30",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY30",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY30",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY30",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY30",
                      "lane": 15
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
                "chip": "IFG1",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY31",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY31",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/32",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY31",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY31",
                        "lane": 13
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/32",
                          "lane": 1
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY31",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY31",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/32",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY31",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY31",
                        "lane": 15
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/32",
                          "lane": 3
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY31",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY31",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/32",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY31",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY31",
                        "lane": 17
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/32",
                          "lane": 5
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY31",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY31",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/32",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY31",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY31",
                        "lane": 19
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/32",
                          "lane": 7
                        }
                      }
                    }
                  ]
                }
              }
            }
          ],
          "portType": 1,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 8
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 8
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 9
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 10
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 11
                    },
                    "tx": {
                      "pre": -76,
                      "pre2": 0,
                      "main": 856,
                      "post": -68,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 4,
                      "dspMode": 7,
                      "afeTrim": 16,
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
                ],
                "xphySys": [
                  {
                    "id": {
                      "chip": "XPHY31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY31",
                      "lane": 3
                    }
                  }
                ],
                "xphyLine": [
                  {
                    "id": {
                      "chip": "XPHY31",
                      "lane": 12
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY31",
                      "lane": 13
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY31",
                      "lane": 14
                    }
                  },
                  {
                    "id": {
                      "chip": "XPHY31",
                      "lane": 15
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
    },
    {
      "name": "XPHY0",
      "type": 2,
      "physicalID": 0
    },
    {
      "name": "XPHY1",
      "type": 2,
      "physicalID": 1
    },
    {
      "name": "XPHY2",
      "type": 2,
      "physicalID": 2
    },
    {
      "name": "XPHY3",
      "type": 2,
      "physicalID": 3
    },
    {
      "name": "XPHY4",
      "type": 2,
      "physicalID": 4
    },
    {
      "name": "XPHY5",
      "type": 2,
      "physicalID": 5
    },
    {
      "name": "XPHY6",
      "type": 2,
      "physicalID": 6
    },
    {
      "name": "XPHY7",
      "type": 2,
      "physicalID": 7
    },
    {
      "name": "XPHY8",
      "type": 2,
      "physicalID": 8
    },
    {
      "name": "XPHY9",
      "type": 2,
      "physicalID": 9
    },
    {
      "name": "XPHY10",
      "type": 2,
      "physicalID": 10
    },
    {
      "name": "XPHY11",
      "type": 2,
      "physicalID": 11
    },
    {
      "name": "XPHY12",
      "type": 2,
      "physicalID": 12
    },
    {
      "name": "XPHY13",
      "type": 2,
      "physicalID": 13
    },
    {
      "name": "XPHY14",
      "type": 2,
      "physicalID": 14
    },
    {
      "name": "XPHY15",
      "type": 2,
      "physicalID": 15
    },
    {
      "name": "XPHY16",
      "type": 2,
      "physicalID": 16
    },
    {
      "name": "XPHY17",
      "type": 2,
      "physicalID": 17
    },
    {
      "name": "XPHY18",
      "type": 2,
      "physicalID": 18
    },
    {
      "name": "XPHY19",
      "type": 2,
      "physicalID": 19
    },
    {
      "name": "XPHY20",
      "type": 2,
      "physicalID": 20
    },
    {
      "name": "XPHY21",
      "type": 2,
      "physicalID": 21
    },
    {
      "name": "XPHY22",
      "type": 2,
      "physicalID": 22
    },
    {
      "name": "XPHY23",
      "type": 2,
      "physicalID": 23
    },
    {
      "name": "XPHY24",
      "type": 2,
      "physicalID": 24
    },
    {
      "name": "XPHY25",
      "type": 2,
      "physicalID": 25
    },
    {
      "name": "XPHY26",
      "type": 2,
      "physicalID": 26
    },
    {
      "name": "XPHY27",
      "type": 2,
      "physicalID": 27
    },
    {
      "name": "XPHY28",
      "type": 2,
      "physicalID": 28
    },
    {
      "name": "XPHY29",
      "type": 2,
      "physicalID": 29
    },
    {
      "name": "XPHY30",
      "type": 2,
      "physicalID": 30
    },
    {
      "name": "XPHY31",
      "type": 2,
      "physicalID": 31
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
          "medium": 2,
          "interfaceMode": 3,
          "interfaceType": 3
        }
      }
    }
  ]
}
)";
} // namespace

namespace facebook {
namespace fboss {
CloudRipperFabricPlatformMapping::CloudRipperFabricPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

CloudRipperFabricPlatformMapping::CloudRipperFabricPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace fboss
} // namespace facebook
