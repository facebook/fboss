/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/makalu/MakaluPlatformMapping.h"

namespace facebook::fboss {
namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
  "ports": {
    "1": {
        "mapping": {
          "id": 1,
          "name": "rcy1/1/1",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "rcy1",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 1
        },
        "supportedProfiles": {
          "11": {
              "subsumedPorts": [

              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "rcy1",
                      "lane": 0
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
          "name": "eth1/1/1",
          "controllingPort": 2,
          "pins": [
            {
              "a": {
                "chip": "BC49",
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
                "chip": "BC49",
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
                "chip": "BC49",
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
                "chip": "BC49",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 2
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
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
                      "chip": "BC49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 3
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
          "name": "eth1/3/1",
          "controllingPort": 3,
          "pins": [
            {
              "a": {
                "chip": "BC50",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC50",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC50",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC50",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 3
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
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
                      "chip": "BC50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
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
          "name": "eth1/4/1",
          "controllingPort": 4,
          "pins": [
            {
              "a": {
                "chip": "BC51",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC51",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC51",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC51",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 4
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
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
                      "chip": "BC51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "5": {
        "mapping": {
          "id": 5,
          "name": "eth1/5/1",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "BC52",
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
                "chip": "BC52",
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
                "chip": "BC52",
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
                "chip": "BC52",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 5
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
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
                      "chip": "BC52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "6": {
        "mapping": {
          "id": 6,
          "name": "eth1/6/1",
          "controllingPort": 6,
          "pins": [
            {
              "a": {
                "chip": "BC53",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC53",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC53",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC53",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 6
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
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
                      "chip": "BC53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "7": {
        "mapping": {
          "id": 7,
          "name": "eth1/7/1",
          "controllingPort": 7,
          "pins": [
            {
              "a": {
                "chip": "BC54",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC54",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC54",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC54",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 7
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
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
                      "chip": "BC54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "8": {
        "mapping": {
          "id": 8,
          "name": "eth1/8/1",
          "controllingPort": 8,
          "pins": [
            {
              "a": {
                "chip": "BC55",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC55",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC55",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC55",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 8
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
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
                      "chip": "BC55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 3
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
          "name": "eth1/9/1",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "BC56",
                "lane": 0
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
                "chip": "BC56",
                "lane": 1
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
                "chip": "BC56",
                "lane": 2
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
                "chip": "BC56",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 9
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
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
                      "chip": "BC56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
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
          "name": "eth1/10/1",
          "controllingPort": 10,
          "pins": [
            {
              "a": {
                "chip": "BC57",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC57",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC57",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC57",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 10
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
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
                      "chip": "BC57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 3
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
          "name": "eth1/11/1",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "BC58",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC58",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC58",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC58",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 11
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
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
                      "chip": "BC58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
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
          "name": "eth1/12/1",
          "controllingPort": 12,
          "pins": [
            {
              "a": {
                "chip": "BC59",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC59",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC59",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC59",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 12
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
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
                      "chip": "BC59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "13": {
        "mapping": {
          "id": 13,
          "name": "eth1/13/1",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "BC60",
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
                "chip": "BC60",
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
                "chip": "BC60",
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
                "chip": "BC60",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 13
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
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
                      "chip": "BC60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "14": {
        "mapping": {
          "id": 14,
          "name": "eth1/14/1",
          "controllingPort": 14,
          "pins": [
            {
              "a": {
                "chip": "BC61",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC61",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC61",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC61",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 14
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
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
                      "chip": "BC61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "15": {
        "mapping": {
          "id": 15,
          "name": "eth1/15/1",
          "controllingPort": 15,
          "pins": [
            {
              "a": {
                "chip": "BC62",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC62",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC62",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC62",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 15
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
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
                      "chip": "BC62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "16": {
        "mapping": {
          "id": 16,
          "name": "eth1/16/1",
          "controllingPort": 16,
          "pins": [
            {
              "a": {
                "chip": "BC63",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC63",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC63",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC63",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 16
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
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
                      "chip": "BC63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
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
          "name": "eth1/17/1",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "BC64",
                "lane": 0
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
                "chip": "BC64",
                "lane": 1
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
                "chip": "BC64",
                "lane": 2
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
                "chip": "BC64",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 17
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC64",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC64",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC64",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC64",
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
                      "chip": "BC64",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC64",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC64",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC64",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "256": {
        "mapping": {
          "id": 256,
          "name": "fab1/256/1",
          "controllingPort": 256,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 0
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "257": {
        "mapping": {
          "id": 257,
          "name": "fab1/257/1",
          "controllingPort": 257,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 1
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "258": {
        "mapping": {
          "id": 258,
          "name": "fab1/258/1",
          "controllingPort": 258,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 2
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "259": {
        "mapping": {
          "id": 259,
          "name": "fab1/259/1",
          "controllingPort": 259,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 3
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "260": {
        "mapping": {
          "id": 260,
          "name": "fab1/260/1",
          "controllingPort": 260,
          "pins": [
            {
              "a": {
                "chip": "BC1",
                "lane": 0
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "261": {
        "mapping": {
          "id": 261,
          "name": "fab1/261/1",
          "controllingPort": 261,
          "pins": [
            {
              "a": {
                "chip": "BC1",
                "lane": 1
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "262": {
        "mapping": {
          "id": 262,
          "name": "fab1/262/1",
          "controllingPort": 262,
          "pins": [
            {
              "a": {
                "chip": "BC1",
                "lane": 2
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "263": {
        "mapping": {
          "id": 263,
          "name": "fab1/263/1",
          "controllingPort": 263,
          "pins": [
            {
              "a": {
                "chip": "BC1",
                "lane": 3
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "264": {
        "mapping": {
          "id": 264,
          "name": "fab1/264/1",
          "controllingPort": 264,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 0
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "265": {
        "mapping": {
          "id": 265,
          "name": "fab1/265/1",
          "controllingPort": 265,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 1
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "266": {
        "mapping": {
          "id": 266,
          "name": "fab1/266/1",
          "controllingPort": 266,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 2
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "267": {
        "mapping": {
          "id": 267,
          "name": "fab1/267/1",
          "controllingPort": 267,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 3
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "268": {
        "mapping": {
          "id": 268,
          "name": "fab1/268/1",
          "controllingPort": 268,
          "pins": [
            {
              "a": {
                "chip": "BC3",
                "lane": 0
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "269": {
        "mapping": {
          "id": 269,
          "name": "fab1/269/1",
          "controllingPort": 269,
          "pins": [
            {
              "a": {
                "chip": "BC3",
                "lane": 1
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 1
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
          "name": "fab1/270/1",
          "controllingPort": 270,
          "pins": [
            {
              "a": {
                "chip": "BC3",
                "lane": 2
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 2
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
          "name": "fab1/271/1",
          "controllingPort": 271,
          "pins": [
            {
              "a": {
                "chip": "BC3",
                "lane": 3
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 3
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
          "name": "fab1/39/1",
          "controllingPort": 272,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/39",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 0
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
          "name": "fab1/39/3",
          "controllingPort": 273,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/39",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "274": {
        "mapping": {
          "id": 274,
          "name": "fab1/39/5",
          "controllingPort": 274,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/39",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "275": {
        "mapping": {
          "id": 275,
          "name": "fab1/39/7",
          "controllingPort": 275,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/39",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "276": {
        "mapping": {
          "id": 276,
          "name": "fab1/37/1",
          "controllingPort": 276,
          "pins": [
            {
              "a": {
                "chip": "BC5",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/37",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "277": {
        "mapping": {
          "id": 277,
          "name": "fab1/37/3",
          "controllingPort": 277,
          "pins": [
            {
              "a": {
                "chip": "BC5",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/37",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "278": {
        "mapping": {
          "id": 278,
          "name": "fab1/37/5",
          "controllingPort": 278,
          "pins": [
            {
              "a": {
                "chip": "BC5",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/37",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "279": {
        "mapping": {
          "id": 279,
          "name": "fab1/37/7",
          "controllingPort": 279,
          "pins": [
            {
              "a": {
                "chip": "BC5",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/37",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "280": {
        "mapping": {
          "id": 280,
          "name": "fab1/33/2",
          "controllingPort": 280,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/33",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "281": {
        "mapping": {
          "id": 281,
          "name": "fab1/33/4",
          "controllingPort": 281,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/33",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "282": {
        "mapping": {
          "id": 282,
          "name": "fab1/33/6",
          "controllingPort": 282,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/33",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "283": {
        "mapping": {
          "id": 283,
          "name": "fab1/33/8",
          "controllingPort": 283,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/33",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "284": {
        "mapping": {
          "id": 284,
          "name": "fab1/35/2",
          "controllingPort": 284,
          "pins": [
            {
              "a": {
                "chip": "BC7",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/35",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "285": {
        "mapping": {
          "id": 285,
          "name": "fab1/35/4",
          "controllingPort": 285,
          "pins": [
            {
              "a": {
                "chip": "BC7",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/35",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "286": {
        "mapping": {
          "id": 286,
          "name": "fab1/35/6",
          "controllingPort": 286,
          "pins": [
            {
              "a": {
                "chip": "BC7",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/35",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "287": {
        "mapping": {
          "id": 287,
          "name": "fab1/35/8",
          "controllingPort": 287,
          "pins": [
            {
              "a": {
                "chip": "BC7",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/35",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 3
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
          "name": "fab1/38/1",
          "controllingPort": 288,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/38",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 0
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
          "name": "fab1/38/3",
          "controllingPort": 289,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/38",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 1
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
          "name": "fab1/38/5",
          "controllingPort": 290,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/38",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 2
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
          "name": "fab1/38/7",
          "controllingPort": 291,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/38",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "292": {
        "mapping": {
          "id": 292,
          "name": "fab1/40/1",
          "controllingPort": 292,
          "pins": [
            {
              "a": {
                "chip": "BC9",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/40",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "293": {
        "mapping": {
          "id": 293,
          "name": "fab1/40/3",
          "controllingPort": 293,
          "pins": [
            {
              "a": {
                "chip": "BC9",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/40",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "294": {
        "mapping": {
          "id": 294,
          "name": "fab1/40/5",
          "controllingPort": 294,
          "pins": [
            {
              "a": {
                "chip": "BC9",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/40",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "295": {
        "mapping": {
          "id": 295,
          "name": "fab1/40/7",
          "controllingPort": 295,
          "pins": [
            {
              "a": {
                "chip": "BC9",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/40",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "296": {
        "mapping": {
          "id": 296,
          "name": "fab1/36/2",
          "controllingPort": 296,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/36",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "297": {
        "mapping": {
          "id": 297,
          "name": "fab1/36/4",
          "controllingPort": 297,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/36",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "298": {
        "mapping": {
          "id": 298,
          "name": "fab1/36/6",
          "controllingPort": 298,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/36",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "299": {
        "mapping": {
          "id": 299,
          "name": "fab1/36/8",
          "controllingPort": 299,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/36",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "300": {
        "mapping": {
          "id": 300,
          "name": "fab1/34/2",
          "controllingPort": 300,
          "pins": [
            {
              "a": {
                "chip": "BC11",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/34",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "301": {
        "mapping": {
          "id": 301,
          "name": "fab1/34/4",
          "controllingPort": 301,
          "pins": [
            {
              "a": {
                "chip": "BC11",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/34",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "302": {
        "mapping": {
          "id": 302,
          "name": "fab1/34/6",
          "controllingPort": 302,
          "pins": [
            {
              "a": {
                "chip": "BC11",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/34",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "303": {
        "mapping": {
          "id": 303,
          "name": "fab1/34/8",
          "controllingPort": 303,
          "pins": [
            {
              "a": {
                "chip": "BC11",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/34",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "304": {
        "mapping": {
          "id": 304,
          "name": "fab1/32/1",
          "controllingPort": 304,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/32",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "305": {
        "mapping": {
          "id": 305,
          "name": "fab1/32/3",
          "controllingPort": 305,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/32",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 1
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
          "name": "fab1/32/5",
          "controllingPort": 306,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/32",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 2
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
          "name": "fab1/32/7",
          "controllingPort": 307,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/32",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 3
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
          "name": "fab1/30/1",
          "controllingPort": 308,
          "pins": [
            {
              "a": {
                "chip": "BC13",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/30",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 0
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
          "name": "fab1/30/3",
          "controllingPort": 309,
          "pins": [
            {
              "a": {
                "chip": "BC13",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/30",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "310": {
        "mapping": {
          "id": 310,
          "name": "fab1/30/5",
          "controllingPort": 310,
          "pins": [
            {
              "a": {
                "chip": "BC13",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/30",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "311": {
        "mapping": {
          "id": 311,
          "name": "fab1/30/7",
          "controllingPort": 311,
          "pins": [
            {
              "a": {
                "chip": "BC13",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/30",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "312": {
        "mapping": {
          "id": 312,
          "name": "fab1/28/2",
          "controllingPort": 312,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/28",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "313": {
        "mapping": {
          "id": 313,
          "name": "fab1/28/4",
          "controllingPort": 313,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/28",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "314": {
        "mapping": {
          "id": 314,
          "name": "fab1/28/6",
          "controllingPort": 314,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/28",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "315": {
        "mapping": {
          "id": 315,
          "name": "fab1/28/8",
          "controllingPort": 315,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/28",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "316": {
        "mapping": {
          "id": 316,
          "name": "fab1/26/2",
          "controllingPort": 316,
          "pins": [
            {
              "a": {
                "chip": "BC15",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/26",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "317": {
        "mapping": {
          "id": 317,
          "name": "fab1/26/4",
          "controllingPort": 317,
          "pins": [
            {
              "a": {
                "chip": "BC15",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/26",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "318": {
        "mapping": {
          "id": 318,
          "name": "fab1/26/6",
          "controllingPort": 318,
          "pins": [
            {
              "a": {
                "chip": "BC15",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/26",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "319": {
        "mapping": {
          "id": 319,
          "name": "fab1/26/8",
          "controllingPort": 319,
          "pins": [
            {
              "a": {
                "chip": "BC15",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/26",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "320": {
        "mapping": {
          "id": 320,
          "name": "fab1/29/1",
          "controllingPort": 320,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/29",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "321": {
        "mapping": {
          "id": 321,
          "name": "fab1/29/3",
          "controllingPort": 321,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/29",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "322": {
        "mapping": {
          "id": 322,
          "name": "fab1/29/5",
          "controllingPort": 322,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/29",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "323": {
        "mapping": {
          "id": 323,
          "name": "fab1/29/7",
          "controllingPort": 323,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/29",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 3
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
          "name": "fab1/31/1",
          "controllingPort": 324,
          "pins": [
            {
              "a": {
                "chip": "BC17",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/31",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 0
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
          "name": "fab1/31/3",
          "controllingPort": 325,
          "pins": [
            {
              "a": {
                "chip": "BC17",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/31",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 1
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
          "name": "fab1/31/5",
          "controllingPort": 326,
          "pins": [
            {
              "a": {
                "chip": "BC17",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/31",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 2
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
          "name": "fab1/31/7",
          "controllingPort": 327,
          "pins": [
            {
              "a": {
                "chip": "BC17",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/31",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "328": {
        "mapping": {
          "id": 328,
          "name": "fab1/25/2",
          "controllingPort": 328,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/25",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "329": {
        "mapping": {
          "id": 329,
          "name": "fab1/25/4",
          "controllingPort": 329,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/25",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "330": {
        "mapping": {
          "id": 330,
          "name": "fab1/25/6",
          "controllingPort": 330,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/25",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "331": {
        "mapping": {
          "id": 331,
          "name": "fab1/25/8",
          "controllingPort": 331,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/25",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "332": {
        "mapping": {
          "id": 332,
          "name": "fab1/27/2",
          "controllingPort": 332,
          "pins": [
            {
              "a": {
                "chip": "BC19",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/27",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "333": {
        "mapping": {
          "id": 333,
          "name": "fab1/27/4",
          "controllingPort": 333,
          "pins": [
            {
              "a": {
                "chip": "BC19",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/27",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "334": {
        "mapping": {
          "id": 334,
          "name": "fab1/27/6",
          "controllingPort": 334,
          "pins": [
            {
              "a": {
                "chip": "BC19",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/27",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "335": {
        "mapping": {
          "id": 335,
          "name": "fab1/27/8",
          "controllingPort": 335,
          "pins": [
            {
              "a": {
                "chip": "BC19",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/27",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "336": {
        "mapping": {
          "id": 336,
          "name": "fab1/21/1",
          "controllingPort": 336,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/21",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "337": {
        "mapping": {
          "id": 337,
          "name": "fab1/21/3",
          "controllingPort": 337,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/21",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "338": {
        "mapping": {
          "id": 338,
          "name": "fab1/21/5",
          "controllingPort": 338,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/21",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "339": {
        "mapping": {
          "id": 339,
          "name": "fab1/21/7",
          "controllingPort": 339,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/21",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "340": {
        "mapping": {
          "id": 340,
          "name": "fab1/23/1",
          "controllingPort": 340,
          "pins": [
            {
              "a": {
                "chip": "BC21",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/23",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "341": {
        "mapping": {
          "id": 341,
          "name": "fab1/23/3",
          "controllingPort": 341,
          "pins": [
            {
              "a": {
                "chip": "BC21",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/23",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 1
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
          "name": "fab1/23/5",
          "controllingPort": 342,
          "pins": [
            {
              "a": {
                "chip": "BC21",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/23",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 2
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
          "name": "fab1/23/7",
          "controllingPort": 343,
          "pins": [
            {
              "a": {
                "chip": "BC21",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/23",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 3
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
          "name": "fab1/17/2",
          "controllingPort": 344,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/17",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 0
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
          "name": "fab1/17/4",
          "controllingPort": 345,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/17",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "346": {
        "mapping": {
          "id": 346,
          "name": "fab1/17/6",
          "controllingPort": 346,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/17",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "347": {
        "mapping": {
          "id": 347,
          "name": "fab1/17/8",
          "controllingPort": 347,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/17",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "348": {
        "mapping": {
          "id": 348,
          "name": "fab1/19/2",
          "controllingPort": 348,
          "pins": [
            {
              "a": {
                "chip": "BC23",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/19",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "349": {
        "mapping": {
          "id": 349,
          "name": "fab1/19/4",
          "controllingPort": 349,
          "pins": [
            {
              "a": {
                "chip": "BC23",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/19",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "350": {
        "mapping": {
          "id": 350,
          "name": "fab1/19/6",
          "controllingPort": 350,
          "pins": [
            {
              "a": {
                "chip": "BC23",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/19",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "351": {
        "mapping": {
          "id": 351,
          "name": "fab1/19/8",
          "controllingPort": 351,
          "pins": [
            {
              "a": {
                "chip": "BC23",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/19",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "352": {
        "mapping": {
          "id": 352,
          "name": "fab1/352/1",
          "controllingPort": 352,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 0
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "353": {
        "mapping": {
          "id": 353,
          "name": "fab1/353/1",
          "controllingPort": 353,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 1
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "354": {
        "mapping": {
          "id": 354,
          "name": "fab1/354/1",
          "controllingPort": 354,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 2
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "355": {
        "mapping": {
          "id": 355,
          "name": "fab1/355/1",
          "controllingPort": 355,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 3
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "356": {
        "mapping": {
          "id": 356,
          "name": "fab1/356/1",
          "controllingPort": 356,
          "pins": [
            {
              "a": {
                "chip": "BC25",
                "lane": 0
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "357": {
        "mapping": {
          "id": 357,
          "name": "fab1/357/1",
          "controllingPort": 357,
          "pins": [
            {
              "a": {
                "chip": "BC25",
                "lane": 1
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "358": {
        "mapping": {
          "id": 358,
          "name": "fab1/358/1",
          "controllingPort": 358,
          "pins": [
            {
              "a": {
                "chip": "BC25",
                "lane": 2
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "359": {
        "mapping": {
          "id": 359,
          "name": "fab1/359/1",
          "controllingPort": 359,
          "pins": [
            {
              "a": {
                "chip": "BC25",
                "lane": 3
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 3
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
          "name": "fab1/360/1",
          "controllingPort": 360,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 0
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 0
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
          "name": "fab1/361/1",
          "controllingPort": 361,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 1
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 1
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
          "name": "fab1/362/1",
          "controllingPort": 362,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 2
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 2
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
          "name": "fab1/363/1",
          "controllingPort": 363,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 3
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "364": {
        "mapping": {
          "id": 364,
          "name": "fab1/364/1",
          "controllingPort": 364,
          "pins": [
            {
              "a": {
                "chip": "BC27",
                "lane": 0
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "365": {
        "mapping": {
          "id": 365,
          "name": "fab1/365/1",
          "controllingPort": 365,
          "pins": [
            {
              "a": {
                "chip": "BC27",
                "lane": 1
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "366": {
        "mapping": {
          "id": 366,
          "name": "fab1/366/1",
          "controllingPort": 366,
          "pins": [
            {
              "a": {
                "chip": "BC27",
                "lane": 2
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "367": {
        "mapping": {
          "id": 367,
          "name": "fab1/367/1",
          "controllingPort": 367,
          "pins": [
            {
              "a": {
                "chip": "BC27",
                "lane": 3
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "368": {
        "mapping": {
          "id": 368,
          "name": "fab1/5/1",
          "controllingPort": 368,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/5",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "369": {
        "mapping": {
          "id": 369,
          "name": "fab1/5/3",
          "controllingPort": 369,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/5",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "370": {
        "mapping": {
          "id": 370,
          "name": "fab1/5/5",
          "controllingPort": 370,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/5",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "371": {
        "mapping": {
          "id": 371,
          "name": "fab1/5/7",
          "controllingPort": 371,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/5",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "372": {
        "mapping": {
          "id": 372,
          "name": "fab1/7/1",
          "controllingPort": 372,
          "pins": [
            {
              "a": {
                "chip": "BC29",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/7",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "373": {
        "mapping": {
          "id": 373,
          "name": "fab1/7/3",
          "controllingPort": 373,
          "pins": [
            {
              "a": {
                "chip": "BC29",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/7",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "374": {
        "mapping": {
          "id": 374,
          "name": "fab1/7/5",
          "controllingPort": 374,
          "pins": [
            {
              "a": {
                "chip": "BC29",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/7",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "375": {
        "mapping": {
          "id": 375,
          "name": "fab1/7/7",
          "controllingPort": 375,
          "pins": [
            {
              "a": {
                "chip": "BC29",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/7",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "376": {
        "mapping": {
          "id": 376,
          "name": "fab1/1/2",
          "controllingPort": 376,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/1",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "377": {
        "mapping": {
          "id": 377,
          "name": "fab1/1/4",
          "controllingPort": 377,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/1",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 1
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
          "name": "fab1/1/6",
          "controllingPort": 378,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/1",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 2
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
          "name": "fab1/1/8",
          "controllingPort": 379,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/1",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 3
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
          "name": "fab1/3/2",
          "controllingPort": 380,
          "pins": [
            {
              "a": {
                "chip": "BC31",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/3",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 0
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
          "name": "fab1/3/4",
          "controllingPort": 381,
          "pins": [
            {
              "a": {
                "chip": "BC31",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/3",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "382": {
        "mapping": {
          "id": 382,
          "name": "fab1/3/6",
          "controllingPort": 382,
          "pins": [
            {
              "a": {
                "chip": "BC31",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/3",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "383": {
        "mapping": {
          "id": 383,
          "name": "fab1/3/8",
          "controllingPort": 383,
          "pins": [
            {
              "a": {
                "chip": "BC31",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/3",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "384": {
        "mapping": {
          "id": 384,
          "name": "fab1/8/1",
          "controllingPort": 384,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/8",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "385": {
        "mapping": {
          "id": 385,
          "name": "fab1/8/3",
          "controllingPort": 385,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/8",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "386": {
        "mapping": {
          "id": 386,
          "name": "fab1/8/5",
          "controllingPort": 386,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/8",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "387": {
        "mapping": {
          "id": 387,
          "name": "fab1/8/7",
          "controllingPort": 387,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/8",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "388": {
        "mapping": {
          "id": 388,
          "name": "fab1/6/1",
          "controllingPort": 388,
          "pins": [
            {
              "a": {
                "chip": "BC33",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/6",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "389": {
        "mapping": {
          "id": 389,
          "name": "fab1/6/3",
          "controllingPort": 389,
          "pins": [
            {
              "a": {
                "chip": "BC33",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/6",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "390": {
        "mapping": {
          "id": 390,
          "name": "fab1/6/5",
          "controllingPort": 390,
          "pins": [
            {
              "a": {
                "chip": "BC33",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/6",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "391": {
        "mapping": {
          "id": 391,
          "name": "fab1/6/7",
          "controllingPort": 391,
          "pins": [
            {
              "a": {
                "chip": "BC33",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/6",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "392": {
        "mapping": {
          "id": 392,
          "name": "fab1/4/2",
          "controllingPort": 392,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/4",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "393": {
        "mapping": {
          "id": 393,
          "name": "fab1/4/4",
          "controllingPort": 393,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/4",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "394": {
        "mapping": {
          "id": 394,
          "name": "fab1/4/6",
          "controllingPort": 394,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/4",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "395": {
        "mapping": {
          "id": 395,
          "name": "fab1/4/8",
          "controllingPort": 395,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/4",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 3
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
          "name": "fab1/2/2",
          "controllingPort": 396,
          "pins": [
            {
              "a": {
                "chip": "BC35",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/2",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 0
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
          "name": "fab1/2/4",
          "controllingPort": 397,
          "pins": [
            {
              "a": {
                "chip": "BC35",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/2",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 1
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
          "name": "fab1/2/6",
          "controllingPort": 398,
          "pins": [
            {
              "a": {
                "chip": "BC35",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/2",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 2
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
          "name": "fab1/2/8",
          "controllingPort": 399,
          "pins": [
            {
              "a": {
                "chip": "BC35",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/2",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "400": {
        "mapping": {
          "id": 400,
          "name": "fab1/16/1",
          "controllingPort": 400,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/16",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "401": {
        "mapping": {
          "id": 401,
          "name": "fab1/16/3",
          "controllingPort": 401,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/16",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "402": {
        "mapping": {
          "id": 402,
          "name": "fab1/16/5",
          "controllingPort": 402,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/16",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "403": {
        "mapping": {
          "id": 403,
          "name": "fab1/16/7",
          "controllingPort": 403,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/16",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "404": {
        "mapping": {
          "id": 404,
          "name": "fab1/14/1",
          "controllingPort": 404,
          "pins": [
            {
              "a": {
                "chip": "BC37",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/14",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "405": {
        "mapping": {
          "id": 405,
          "name": "fab1/14/3",
          "controllingPort": 405,
          "pins": [
            {
              "a": {
                "chip": "BC37",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/14",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "406": {
        "mapping": {
          "id": 406,
          "name": "fab1/14/5",
          "controllingPort": 406,
          "pins": [
            {
              "a": {
                "chip": "BC37",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/14",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "407": {
        "mapping": {
          "id": 407,
          "name": "fab1/14/7",
          "controllingPort": 407,
          "pins": [
            {
              "a": {
                "chip": "BC37",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/14",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "408": {
        "mapping": {
          "id": 408,
          "name": "fab1/12/2",
          "controllingPort": 408,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/12",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "409": {
        "mapping": {
          "id": 409,
          "name": "fab1/12/4",
          "controllingPort": 409,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/12",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "410": {
        "mapping": {
          "id": 410,
          "name": "fab1/12/6",
          "controllingPort": 410,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/12",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "411": {
        "mapping": {
          "id": 411,
          "name": "fab1/12/8",
          "controllingPort": 411,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/12",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "412": {
        "mapping": {
          "id": 412,
          "name": "fab1/10/2",
          "controllingPort": 412,
          "pins": [
            {
              "a": {
                "chip": "BC39",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/10",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "413": {
        "mapping": {
          "id": 413,
          "name": "fab1/10/4",
          "controllingPort": 413,
          "pins": [
            {
              "a": {
                "chip": "BC39",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/10",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 1
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
          "name": "fab1/10/6",
          "controllingPort": 414,
          "pins": [
            {
              "a": {
                "chip": "BC39",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/10",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 2
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
          "name": "fab1/10/8",
          "controllingPort": 415,
          "pins": [
            {
              "a": {
                "chip": "BC39",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/10",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 3
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
          "name": "fab1/9/2",
          "controllingPort": 416,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/9",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 0
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
          "name": "fab1/9/4",
          "controllingPort": 417,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/9",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "418": {
        "mapping": {
          "id": 418,
          "name": "fab1/9/6",
          "controllingPort": 418,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/9",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "419": {
        "mapping": {
          "id": 419,
          "name": "fab1/9/8",
          "controllingPort": 419,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/9",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "420": {
        "mapping": {
          "id": 420,
          "name": "fab1/11/2",
          "controllingPort": 420,
          "pins": [
            {
              "a": {
                "chip": "BC41",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/11",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "421": {
        "mapping": {
          "id": 421,
          "name": "fab1/11/4",
          "controllingPort": 421,
          "pins": [
            {
              "a": {
                "chip": "BC41",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/11",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "422": {
        "mapping": {
          "id": 422,
          "name": "fab1/11/6",
          "controllingPort": 422,
          "pins": [
            {
              "a": {
                "chip": "BC41",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/11",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "423": {
        "mapping": {
          "id": 423,
          "name": "fab1/11/8",
          "controllingPort": 423,
          "pins": [
            {
              "a": {
                "chip": "BC41",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/11",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "424": {
        "mapping": {
          "id": 424,
          "name": "fab1/15/1",
          "controllingPort": 424,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/15",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "425": {
        "mapping": {
          "id": 425,
          "name": "fab1/15/3",
          "controllingPort": 425,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/15",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "426": {
        "mapping": {
          "id": 426,
          "name": "fab1/15/5",
          "controllingPort": 426,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/15",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "427": {
        "mapping": {
          "id": 427,
          "name": "fab1/15/7",
          "controllingPort": 427,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/15",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "428": {
        "mapping": {
          "id": 428,
          "name": "fab1/13/1",
          "controllingPort": 428,
          "pins": [
            {
              "a": {
                "chip": "BC43",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/13",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "429": {
        "mapping": {
          "id": 429,
          "name": "fab1/13/3",
          "controllingPort": 429,
          "pins": [
            {
              "a": {
                "chip": "BC43",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/13",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "430": {
        "mapping": {
          "id": 430,
          "name": "fab1/13/5",
          "controllingPort": 430,
          "pins": [
            {
              "a": {
                "chip": "BC43",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/13",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "431": {
        "mapping": {
          "id": 431,
          "name": "fab1/13/7",
          "controllingPort": 431,
          "pins": [
            {
              "a": {
                "chip": "BC43",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/13",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 3
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
          "name": "fab1/18/2",
          "controllingPort": 432,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/18",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 0
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
          "name": "fab1/18/4",
          "controllingPort": 433,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/18",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 1
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
          "name": "fab1/18/6",
          "controllingPort": 434,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/18",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 2
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
          "name": "fab1/18/8",
          "controllingPort": 435,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/18",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "436": {
        "mapping": {
          "id": 436,
          "name": "fab1/20/2",
          "controllingPort": 436,
          "pins": [
            {
              "a": {
                "chip": "BC45",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/20",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "437": {
        "mapping": {
          "id": 437,
          "name": "fab1/20/4",
          "controllingPort": 437,
          "pins": [
            {
              "a": {
                "chip": "BC45",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/20",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "438": {
        "mapping": {
          "id": 438,
          "name": "fab1/20/6",
          "controllingPort": 438,
          "pins": [
            {
              "a": {
                "chip": "BC45",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/20",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "439": {
        "mapping": {
          "id": 439,
          "name": "fab1/20/8",
          "controllingPort": 439,
          "pins": [
            {
              "a": {
                "chip": "BC45",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/20",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "440": {
        "mapping": {
          "id": 440,
          "name": "fab1/22/1",
          "controllingPort": 440,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/22",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "441": {
        "mapping": {
          "id": 441,
          "name": "fab1/22/3",
          "controllingPort": 441,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/22",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "442": {
        "mapping": {
          "id": 442,
          "name": "fab1/22/5",
          "controllingPort": 442,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/22",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "443": {
        "mapping": {
          "id": 443,
          "name": "fab1/22/7",
          "controllingPort": 443,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/22",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "444": {
        "mapping": {
          "id": 444,
          "name": "fab1/24/1",
          "controllingPort": 444,
          "pins": [
            {
              "a": {
                "chip": "BC47",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/24",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "445": {
        "mapping": {
          "id": 445,
          "name": "fab1/24/3",
          "controllingPort": 445,
          "pins": [
            {
              "a": {
                "chip": "BC47",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/24",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "446": {
        "mapping": {
          "id": 446,
          "name": "fab1/24/5",
          "controllingPort": 446,
          "pins": [
            {
              "a": {
                "chip": "BC47",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/24",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "447": {
        "mapping": {
          "id": 447,
          "name": "fab1/24/7",
          "controllingPort": 447,
          "pins": [
            {
              "a": {
                "chip": "BC47",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/24",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
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
      "name": "BC0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "BC1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "BC2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "BC3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "BC4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "BC5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "BC6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "BC7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "BC8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "BC9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "BC10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "BC11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "BC12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "BC13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "BC14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "BC15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "BC16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "BC17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "BC18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "BC19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "BC20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "BC21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "BC22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "BC23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "BC24",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "BC25",
      "type": 1,
      "physicalID": 25
    },
    {
      "name": "BC26",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "BC27",
      "type": 1,
      "physicalID": 27
    },
    {
      "name": "BC28",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "BC29",
      "type": 1,
      "physicalID": 29
    },
    {
      "name": "BC30",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "BC31",
      "type": 1,
      "physicalID": 31
    },
    {
      "name": "BC32",
      "type": 1,
      "physicalID": 32
    },
    {
      "name": "BC33",
      "type": 1,
      "physicalID": 33
    },
    {
      "name": "BC34",
      "type": 1,
      "physicalID": 34
    },
    {
      "name": "BC35",
      "type": 1,
      "physicalID": 35
    },
    {
      "name": "BC36",
      "type": 1,
      "physicalID": 36
    },
    {
      "name": "BC37",
      "type": 1,
      "physicalID": 37
    },
    {
      "name": "BC38",
      "type": 1,
      "physicalID": 38
    },
    {
      "name": "BC39",
      "type": 1,
      "physicalID": 39
    },
    {
      "name": "BC40",
      "type": 1,
      "physicalID": 40
    },
    {
      "name": "BC41",
      "type": 1,
      "physicalID": 41
    },
    {
      "name": "BC42",
      "type": 1,
      "physicalID": 42
    },
    {
      "name": "BC43",
      "type": 1,
      "physicalID": 43
    },
    {
      "name": "BC44",
      "type": 1,
      "physicalID": 44
    },
    {
      "name": "BC45",
      "type": 1,
      "physicalID": 45
    },
    {
      "name": "BC46",
      "type": 1,
      "physicalID": 46
    },
    {
      "name": "BC47",
      "type": 1,
      "physicalID": 47
    },
    {
      "name": "BC64",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "BC49",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "BC50",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "BC51",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "BC52",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "BC53",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "BC54",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "BC55",
      "type": 1,
      "physicalID": 32
    },
    {
      "name": "BC56",
      "type": 1,
      "physicalID": 34
    },
    {
      "name": "BC57",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "BC58",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "BC59",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "BC60",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "BC61",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "BC62",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "BC63",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "fab1/1",
      "type": 3,
      "physicalID": 36
    },
    {
      "name": "fab1/2",
      "type": 3,
      "physicalID": 37
    },
    {
      "name": "fab1/3",
      "type": 3,
      "physicalID": 38
    },
    {
      "name": "fab1/4",
      "type": 3,
      "physicalID": 39
    },
    {
      "name": "fab1/5",
      "type": 3,
      "physicalID": 40
    },
    {
      "name": "fab1/6",
      "type": 3,
      "physicalID": 41
    },
    {
      "name": "fab1/7",
      "type": 3,
      "physicalID": 42
    },
    {
      "name": "fab1/8",
      "type": 3,
      "physicalID": 43
    },
    {
      "name": "fab1/9",
      "type": 3,
      "physicalID": 44
    },
    {
      "name": "fab1/10",
      "type": 3,
      "physicalID": 45
    },
    {
      "name": "fab1/11",
      "type": 3,
      "physicalID": 46
    },
    {
      "name": "fab1/12",
      "type": 3,
      "physicalID": 47
    },
    {
      "name": "fab1/13",
      "type": 3,
      "physicalID": 48
    },
    {
      "name": "fab1/14",
      "type": 3,
      "physicalID": 49
    },
    {
      "name": "fab1/15",
      "type": 3,
      "physicalID": 50
    },
    {
      "name": "fab1/16",
      "type": 3,
      "physicalID": 51
    },
    {
      "name": "fab1/17",
      "type": 3,
      "physicalID": 52
    },
    {
      "name": "fab1/18",
      "type": 3,
      "physicalID": 53
    },
    {
      "name": "fab1/19",
      "type": 3,
      "physicalID": 54
    },
    {
      "name": "fab1/20",
      "type": 3,
      "physicalID": 55
    },
    {
      "name": "fab1/21",
      "type": 3,
      "physicalID": 56
    },
    {
      "name": "fab1/22",
      "type": 3,
      "physicalID": 57
    },
    {
      "name": "fab1/23",
      "type": 3,
      "physicalID": 58
    },
    {
      "name": "fab1/24",
      "type": 3,
      "physicalID": 59
    },
    {
      "name": "fab1/25",
      "type": 3,
      "physicalID": 60
    },
    {
      "name": "fab1/26",
      "type": 3,
      "physicalID": 61
    },
    {
      "name": "fab1/27",
      "type": 3,
      "physicalID": 62
    },
    {
      "name": "fab1/28",
      "type": 3,
      "physicalID": 63
    },
    {
      "name": "fab1/29",
      "type": 3,
      "physicalID": 64
    },
    {
      "name": "fab1/30",
      "type": 3,
      "physicalID": 65
    },
    {
      "name": "fab1/31",
      "type": 3,
      "physicalID": 66
    },
    {
      "name": "fab1/32",
      "type": 3,
      "physicalID": 67
    },
    {
      "name": "fab1/33",
      "type": 3,
      "physicalID": 68
    },
    {
      "name": "fab1/34",
      "type": 3,
      "physicalID": 69
    },
    {
      "name": "fab1/35",
      "type": 3,
      "physicalID": 70
    },
    {
      "name": "fab1/36",
      "type": 3,
      "physicalID": 71
    },
    {
      "name": "fab1/37",
      "type": 3,
      "physicalID": 72
    },
    {
      "name": "fab1/38",
      "type": 3,
      "physicalID": 73
    },
    {
      "name": "fab1/39",
      "type": 3,
      "physicalID": 74
    },
    {
      "name": "fab1/40",
      "type": 3,
      "physicalID": 75
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
      "name": "rcy1",
      "type": 1,
      "physicalID": 55
    }
  ],
  "platformSettings": {
    "1": "68:00"
  },
  "portConfigOverrides": [
    {
      "factor": {
        "ports": [
          376,
          377,
          378,
          379,
          396,
          397,
          398,
          399,
          380,
          381,
          382,
          383,
          392,
          393,
          394,
          395,
          368,
          369,
          370,
          371,
          388,
          389,
          390,
          391,
          372,
          373,
          374,
          375,
          384,
          385,
          386,
          387,
          416,
          417,
          418,
          419,
          412,
          413,
          414,
          415,
          420,
          421,
          422,
          423,
          408,
          409,
          410,
          411,
          428,
          429,
          430,
          431,
          404,
          405,
          406,
          407,
          424,
          425,
          426,
          427,
          400,
          401,
          402,
          403,
          344,
          345,
          346,
          347,
          432,
          433,
          434,
          435,
          348,
          349,
          350,
          351,
          436,
          437,
          438,
          439,
          336,
          337,
          338,
          339,
          440,
          441,
          442,
          443,
          340,
          341,
          342,
          343,
          444,
          445,
          446,
          447,
          328,
          329,
          330,
          331,
          316,
          317,
          318,
          319,
          332,
          333,
          334,
          335,
          312,
          313,
          314,
          315,
          320,
          321,
          322,
          323,
          308,
          309,
          310,
          311,
          324,
          325,
          326,
          327,
          304,
          305,
          306,
          307,
          280,
          281,
          282,
          283,
          300,
          301,
          302,
          303,
          284,
          285,
          286,
          287,
          296,
          297,
          298,
          299,
          276,
          277,
          278,
          279,
          288,
          289,
          290,
          291,
          272,
          273,
          274,
          275,
          292,
          293,
          294,
          295,
          256,
          257,
          258,
          259,
          260,
          261,
          262,
          263,
          264,
          265,
          266,
          267,
          268,
          269,
          270,
          271,
          352,
          353,
          354,
          355,
          356,
          357,
          358,
          359,
          360,
          361,
          362,
          363,
          364,
          365,
          366,
          367
        ],
        "profiles": [
          36,
          37
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": -16,
              "pre2": 0,
              "main": 132,
              "post": 0,
              "post2": 0,
              "post3": 0
            }
          }
        ]
      }
    }
  ],
  "platformSupportedProfiles": [
    {
      "factor": {
        "profileID": 36
      },
      "profile": {
        "speed": 53125,
        "iphy": {
          "numLanes": 1,
          "modulation": 2,
          "fec": 545,
          "medium": 1,
          "interfaceMode": 41,
          "interfaceType": 41
        }
      }
    },
    {
      "factor": {
        "profileID": 37
      },
      "profile": {
        "speed": 53125,
        "iphy": {
          "numLanes": 1,
          "modulation": 2,
          "fec": 545,
          "medium": 3,
          "interfaceMode": 41,
          "interfaceType": 41
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
    }
  ]
}
)";

} // namespace

MakaluPlatformMapping::MakaluPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

MakaluPlatformMapping::MakaluPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
